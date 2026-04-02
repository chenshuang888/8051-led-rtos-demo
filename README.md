# 8051（Keil C51）协作式“伪 RTOS”+ 分层框架流水灯示例

这是一个运行在 51 单片机（以 AT89C51/REGX52 环境为代表）上的**极简协作式调度框架**示例工程，包含：

- **1ms 时基（Timer0 中断产生 tick）**
- **协作式调度器**（主循环轮询，任务按周期执行）
- **小型队列（OS Queue）**（用于任务间消息传递，尽量节省 RAM）
- **按键三层架构（BSP / OBJ / APP）**：P3.0~P3.3 低电平有效
- **LED 三层架构（BSP / OBJ / APP）**：P2.0~P2.7 低电平点亮
- **LVGL 风格的“对象 + handler”写法**：状态放在对象里，定时 `handler()` 推进

工程的目标不是“完整 RTOS”，而是给 8051 这种 **RAM 极小（典型 128B IRAM）** 的平台提供一种**足够好用、结构清晰、可扩展**的工程骨架。

---

## 快速开始（Keil uVision）

1. 用 Keil uVision 打开工程文件：`keil/Project.uvproj`
2. `Build Target` 编译
3. 下载到单片机（或仿真）运行

### 硬件假设

- **LED**：接在 `P2.0 ~ P2.7`，并且 **低电平点亮**（active-low）
- **按键**：接在 `P3.0 ~ P3.3`，并且 **低电平有效**（按下读到 0）
- 晶振频率默认按 `config.h` 的 `FOSC_HZ` 配置（本工程默认 24MHz）

如果你的板子晶振不是 24MHz，请优先修改 `config.h` 中的 `FOSC_HZ`，否则 Timer0 的 1ms tick 会不准。

---

## 运行现象（默认映射）

上电后：

- LED 默认**单灯流水**，周期约 500ms（注意：第一次点亮会在约 0.5s 后出现，这是设计使然）

按键功能（只处理短按 CLICK，长按 LONG 当前不绑定 LED 行为）：

- **K0（P3.0）**：切换流水方向（正向/反向）
- **K1（P3.1）**：加速（周期 -50ms，下限 100ms）
- **K2（P3.2）**：减速（周期 +50ms，上限 1950ms）
- **K3（P3.3）**：暂停/恢复（暂停时保持当前灯不动）

---

## 目录结构（分层）

工程按“OS / BSP / OBJ / APP”分层组织，根目录保持尽量干净：

```
.
├─ keil/                 # Keil 工程文件与界面状态（uvproj/uvopt/uvgui）
├─ os/                   # OS：tick / scheduler / queue 等基础设施
├─ bsp/                  # BSP：唯一允许直接读写寄存器(Px/Tx/中断)的层
├─ obj/                  # 框架/对象层：状态机、对象、handler（不碰硬件）
├─ app/                  # 应用层：粘合、语义映射、组装消息、业务策略
├─ main.c                # 入口：初始化 + 创建任务 + while(1) 调度
├─ config.h              # 时钟/定时器相关配置
└─ types.h               # u8/u16/bit 等基础类型
```

### 分层原则（很重要）

- `bsp/`：只做“怎么读/怎么写硬件”，不做策略；例如 LED 的 P2 写入、按键的 P3 读取。
- `obj/`：只做“状态机/对象/事件分发/handler 推进”，不直接访问寄存器。
- `app/`：把多个模块粘起来（例如把按键消息映射成 LED 语义事件）。
- `os/`：提供最小可用的调度与队列，避免引入复杂内核占用 RAM。

---

## 代码走读（从上到下）

### 1) main：初始化 + 创建任务

文件：`main.c`

- `os_init()`：初始化协作调度器任务表
- `app_key_init()`：初始化按键模块（包含队列初始化、注册按键读取回调）
- `app_led_init()`：初始化 LED 模块（绑定时钟源、绑定输出、注册事件处理）
- `os_task_create(app_led_task, 10)`：LED 任务 10ms 周期
- `os_task_create(app_key_task, 10)`：Key 任务 10ms 周期
- `bsp_timer0_init()`：打开 Timer0，开始产生 1ms tick
- `while(1) os_run_once()`：协作式调度循环

> 任务创建顺序会影响同周期内的执行顺序：本工程 LED 任务先跑、Key 任务后跑，因此按键事件通常会在下一次 LED 任务运行时被消费（约 10~20ms 延迟，肉眼不可见）。

### 2) 时基：Timer0 产生 1ms tick

文件：`bsp/bsp_timer0.c`

- `timer0_isr()`：每 1ms 进入一次中断，重装 TH0/TL0，并执行 `g_os_tick++`
- `os_tick_now()`：读取 16 位 tick 时**临时关中断**，避免 8051 读取 16 位时被中断打断造成“撕裂读”

这里的 tick 是系统里所有“时间”的来源：

- OS 调度器用它判断任务是否到期
- LED 框架用它计算 dt（与 LVGL 的 `lv_timer_handler()` 类似）

### 3) OS：协作式调度器（Minimal Scheduler）

文件：`os/os.c`、`os/os.h`

核心思路：

- 每个任务记录 `{fn, rate_ms, last_run}`
- 主循环不断读取 `now = os_tick_now()`，判断 `(u16)(now - last_run) >= rate_ms` 是否到期
- 到期则执行任务，并用 `last_run += rate_ms` 方式保持“更稳定的节拍”（减少漂移）

这是一种典型的**协作式（cooperative）**方式：

- 任务函数必须**尽量短小**，不能长时间阻塞
- 如果任务要做复杂工作，请拆分成多步，在多次周期里完成

### 4) OS Queue：小型消息队列（任务间通信）

文件：`os/os_queue.c`、`os/os_queue.h`

特点：

- 存储由应用层提供静态数组（避免 OS 内部动态分配）
- `send/recv` 通过临界区保护，适合在 8051 上使用
- 用于 Key -> LED 的消息传递：`g_key_queue`

### 5) Key：按键三层架构

**BSP（读硬件）**：`bsp/bsp_key.c`、`bsp/bsp_key.h`

- `bsp_key_read_mask()`：返回 `pressed_mask`，bit i=1 表示按下（已做 active-low 归一化）

**OBJ（状态机）**：`obj/key_obj.c`、`obj/key_obj.h`

- UP/FALL/DOWN/RISE 四态，属于“最小去抖 + 松开确认输出事件”的写法
- 长按阈值：`KEY_LONG_TICKS=80`，当 Key 任务为 10ms 周期时约等于 800ms
- `key_obj_attach_reader()`：注册输入读取回调（无参返回 mask，尽量 C51 友好）
- `key_obj_poll()`：内部读取 mask 并扫描所有键，**一次最多吐出一个事件**

**APP（组包入队）**：`app/app_key.c`、`app/app_key.h`

- `app_key_task()`：循环 `key_obj_poll()` 读空事件，然后组装 `key_msg_t {key_id, action}` 并入队 `g_key_queue`

### 6) LED：LVGL 风格对象框架

**BSP（写硬件）**：`bsp/bsp_led.c`、`bsp/bsp_led.h`

- `bsp_led_write_mask(logical_mask)`：bit i=1 表示第 i 个 LED 亮，内部做 active-low 反相并写到 P2

**OBJ（对象与 handler）**：`obj/led_obj.c`、`obj/led_obj.h`

- `led_t` 保存所有状态：`idx/dir/running/period_ms/acc_ms/last_tick/...`
- `led_bind_clock()` 注入 `now_ms()`（本工程绑定 `os_tick_now`）
- `led_bind_output()` 注入 `write_mask()`（本工程绑定 `bsp_led_write_mask`）
- `led_feed_evt()` 输入语义事件（LED_EVT_*），内部 dispatch 到绑定表
- `led_handler()` 每次调用根据 `now_ms()` 计算 dt，累加到 `acc_ms`，到期执行一步 `led_step()`

**APP（语义映射 + 绑定回调）**：`app/app_led.c`、`app/app_led.h`

- 从 `g_key_queue` 取 `key_msg_t`
- 把 `(key_id, action)` 映射为 `LED_EVT_*`（表驱动：`s_keymap[]`）
- 把 `LED_EVT_*` 喂给 `led_feed_evt(&g_led, evt_id)`
- `s_led_handlers[]` 定义 `LED_EVT_* -> callback`，回调只修改对象内部状态（方向、速度、暂停）

---

## 如何扩展（建议路线）

### 1) 新增一个“设备”

比如蜂鸣器/数码管/串口：

1. `bsp/` 加一个 `bsp_xxx.c/.h`：只负责寄存器读写
2. `obj/` 加一个 `xxx_obj.c/.h`：只做对象状态与 handler
3. `app/` 加一个 `app_xxx.c/.h`：做粘合（绑定 clock/output/handlers，和别的模块交互）

### 2) 新增更多按键语义

目前 Key OBJ 只输出 `CLICK/LONG` 两种动作。你可以在 `obj/key_obj.c` 增加更多 action（例如 DOUBLE_CLICK），但注意：

- 8051 RAM 极小：避免在 OBJ 内引入大缓存
- 建议优先用“更小的状态机变量”实现，而不是上队列存大量历史

---

## 常见问题（FAQ）

### Q1：为什么读取 16 位 tick 要关中断？

8051 是 8 位机，读 16 位变量会分两次读。如果中间被中断打断（tick++），就可能读到“高字节旧 + 低字节新”的撕裂值，导致时间判断偶发错误。`os_tick_now()` 用临界区保证读到一致值。

### Q2：为什么第一次点亮要等一段时间？

`led_init()` 默认 `period_ms=500`，且 `led_handler()` 是“acc_ms 累加到 period 才 step”，因此上电后前约 0.5s 会保持全灭，然后才开始第一步。

### Q3：暂停为什么是“停住不动”而不是“全灭”？

当前策略是 `running=0` 时 `led_handler()` 直接 return，不主动清输出。这样暂停保持当前显示状态更直观。如果你想暂停时全灭，可以在暂停事件回调里调用 `write_mask(0)`（在 APP/OBJ 层都可以实现）。

---

## 开源建议

- 建议把 `keil/Project.uvgui.*` 加入忽略（这是个人界面状态文件，多人协作会互相覆盖）。
- `Objects/`、`Listings/` 也建议忽略（编译产物）。

（如果你需要，我可以补一份适合 GitHub 的 `.gitignore` 模板。）

