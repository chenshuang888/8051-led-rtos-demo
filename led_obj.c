#include "led_obj.h"

/*
 * led_obj.c
 *
 * 这是 LED “对象框架”的实现文件。
 *
 * 整体思路非常像 LVGL：
 * - 回调（handlers）只负责改对象状态（dir/period/running 等）
 * - handler 负责根据时间推进，必要时渲染（输出）到硬件
 *
 * 输出采用“逻辑掩码”logical_mask：bit i=1 表示第 i 个灯应该亮。
 * 真正怎么写到 P2、是否反相、是否有其它硬件映射，都由应用层注入的
 * write_mask() 决定。
 */

static void led_render(led_t *s)
{
       u8 logical_mask;

       if (s->write_mask == 0)
       {
               return;
       }

       /* logical_mask bit i => LED i should be ON */
       logical_mask = (u8)(1u << (s->idx & 0x07));
       s->write_mask(logical_mask);
}

/*
 * 步进一次（走一步灯）。
 * - 先渲染当前 idx
 * - 再根据 dir 更新 idx
 */
static void led_step(led_t *s)
{
       led_render(s);

       if (s->dir == 0)
       {
               s->idx = (u8)((s->idx + 1) & 0x07);
       }
       else
       {
               s->idx = (u8)((s->idx + 7) & 0x07);
       }
}

/*
 * 语义事件分发：遍历 evt_bindings 表，命中 id 就调用回调。
 *
 * 说明：
 * - evt_bindings 指向 code 段常量表，访问时用 `code *` 指针类型。
 * - 允许多个条目使用同一个 evt_id，实现“多监听者”。
 */
static void led_dispatch_evt(led_t *s, u8 evt_id)
{
       u8 i;
       if (evt_id == LED_EVT_NONE)
       {
               return;
       }
       if (s->evt_bindings == 0 || s->evt_binding_count == 0)
       {
               return;
       }

       /* LVGL-style: allow multiple handlers to listen to the same event id. */
       for (i = 0; i < s->evt_binding_count; i++)
       {
               const led_evt_binding_t code *b = &s->evt_bindings[i];
               if (b->id == evt_id && b->cb)
               {
                       b->cb(s);
               }
       }
}

/*
 * led 对象初始化：设置默认状态并清空所有注入接口/绑定表。
 *
 * 默认行为：
 * - idx=0，dir=0(正向)，running=1
 * - period_ms=500
 */
void led_init(led_t *s)
{
       s->idx = 0;
       s->dir = 0;
       s->running = 1;
       s->period_ms = 500;

       s->acc_ms = 0;
       s->last_tick = 0;
       s->now_ms = 0;
       s->write_mask = 0;
       s->map_key = 0;
       s->evt_bindings = 0;
       s->evt_binding_count = 0;
}

/*
 * 注入时钟源（ms）：一般绑定为 os_tick_now。
 *
 * 注意：
 * - 绑定后会立即读取一次 now_ms 作为 last_tick 的初值；
 * - 这样第一次进入 led_handler 时 dt 不会异常大。
 */
void led_bind_clock(led_t *s, led_now_ms_fn_t now_ms)
{
       s->now_ms = now_ms;
       s->last_tick = now_ms ? now_ms() : 0;
}

/*
 * 注入输出函数 write_mask。
 *
 * 约定：
 * - logical_mask bit i=1 表示第 i 个灯“逻辑上应该亮”。
 * - 至于是否需要取反、如何映射到 IO 口，由应用层 write_mask 自己决定。
 *
 * 绑定时会清屏：write_mask(0)。
 */
void led_bind_output(led_t *s, led_write_mask_fn_t write_mask)
{
       s->write_mask = write_mask;

       /* Clear output on bind. */
       if (write_mask)
       {
               write_mask(0);
       }
}

/*
 * 注入按键映射函数 map_key：raw key -> 语义事件。
 *
 * 输入：key_id + action（来自 app_key 的 key_msg_t）
 * 输出：LED_EVT_*（不关心则返回 LED_EVT_NONE）
 */
void led_bind_key_mapper(led_t *s, led_key_map_fn_t map_key)
{
       s->map_key = map_key;
}

/*
 * 注入事件绑定表：evt_id -> callback。
 *
 * 说明：
 * - bindings 建议放在 code 段（用 `code` 关键字），节省 DATA。
 * - count 是条目数，不是字节数。
 */
void led_bind_event_handlers(led_t *s, const led_evt_binding_t code *bindings, u8 count)
{
       s->evt_bindings = bindings;
       s->evt_binding_count = count;
}

/*
 * 喂入按键信息：内部调用 map_key 得到语义事件，再分发给 handlers。
 *
 * 说明：
 * - 这里是“输入层”，只负责：raw -> semantic -> dispatch；
 * - 具体怎么改 led 状态，由 handlers 回调决定。
 */
void led_feed_key(led_t *s, const key_msg_t *msg)
{
       if (msg == 0 || s->map_key == 0)
       {
               return;
       }

       {
               u8 evt_id = s->map_key(msg->key_id, msg->action);
               if (evt_id != LED_EVT_NONE)
               {
                       led_dispatch_evt(s, evt_id);
               }
       }
}

/*
 * LED 主处理函数（类似 LVGL handler）：
 * - 读取 now_ms() 计算 dt
 * - 累计 acc_ms，到点则 step 一次（并通过 write_mask 输出）
 *
 * 注意：
 * - dt 使用无符号减法，tick 回绕安全；
 * - running=0 时不推进，但仍更新 last_tick，避免恢复后 dt 巨大。
 */
void led_handler(led_t *s)
{
       u16 now;
       u16 dt;

       if (s->now_ms == 0)
       {
               return;
       }

       now = s->now_ms();
       dt = (u16)(now - s->last_tick); /* 无符号回绕安全（tick 溢出也没问题） */
       s->last_tick = now;

       /* 暂停时也要更新 last_tick（上面已做），避免恢复后 dt 突然变得很大。 */
       if (!s->running)
       {
               return;
       }

       s->acc_ms = (u16)(s->acc_ms + dt);
       if (s->acc_ms >= s->period_ms)
       {
               s->acc_ms = 0;
               led_step(s);
       }
}
