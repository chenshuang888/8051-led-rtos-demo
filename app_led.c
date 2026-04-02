#include <REGX52.H>

#include "app_led.h"
#include "app_key.h"
#include "bsp_timer0.h"
#include "led_obj.h"
#include "os_queue.h"

/*
 * app_led.c
 *
 * LED 应用层：把“按键队列”接入到 LED 框架，并定义：
 * - raw key (key_id + action) 到 LED 语义事件的映射表 s_keymap[]
 * - LED 语义事件到回调函数的绑定表 s_led_handlers[]
 * - 硬件输出函数 led_write_mask_p2()（将 logical_mask 写到 P2）
 *
 * 约定：
 * - LED 在 P2.0~P2.7，低电平点亮（active-low）。
 * - 语义事件目前只处理 CLICK；如需 LONG 可在 s_keymap 里扩展。
 */

static led_t g_led;

/* raw key -> LED 语义事件 映射表条目 */
typedef struct
{
	u8 key_id;
	u8 action; /* KEY_ACT_* */
	u8 evt_id; /* LED_EVT_* */
} key_to_led_map_t;

/*
 * raw key 映射表：
 * - key_id/action 来自 app_key
 * - evt_id 是 LED 框架内部语义事件
 *
 * 放到 code 段：减少 DATA 占用（8051 RAM 很小）。
 */
static const key_to_led_map_t code s_keymap[] = {
	{0, KEY_ACT_CLICK, LED_EVT_TOGGLE_DIR},
	{1, KEY_ACT_CLICK, LED_EVT_SPEED_UP},
	{2, KEY_ACT_CLICK, LED_EVT_SPEED_DOWN},
	{3, KEY_ACT_CLICK, LED_EVT_TOGGLE_RUN},
};

/*
 * 按键消息 -> LED 语义事件 的映射函数（注入给 led_obj）。
 *
 * 输入：
 * - key_id: 0..3
 * - action: KEY_ACT_CLICK / KEY_ACT_LONG
 *
 * 输出：
 * - 返回 LED_EVT_*，若不关心该组合则返回 LED_EVT_NONE。
 *
 * 设计原因：
 * - LED 框架不应该“认识具体按键含义”，它只认识语义事件；
 * - 语义映射由应用层决定（更像 LVGL：indev -> event）。
 */
static u8 app_led_map_from_key(const key_msg_t *msg)
{
	u8 i;

	if (msg == 0)
	{
		return LED_EVT_NONE;
	}

	for (i = 0; i < (u8)(sizeof(s_keymap) / sizeof(s_keymap[0])); i++)
	{
		if (s_keymap[i].key_id == msg->key_id && s_keymap[i].action == msg->action)
		{
			return s_keymap[i].evt_id;
		}
	}
	return LED_EVT_NONE;
}

/* K0: 反转方向（正向/反向） */
static void on_toggle_dir(led_t *s)
{
	s->dir = (u8)(!s->dir);
}

/* K1: 加速（缩短步进周期） */
static void on_speed_up(led_t *s)
{
	if (s->period_ms > 100)
	{
		s->period_ms = (u16)(s->period_ms - 50);
	}
}

/* K2: 减速（增加步进周期） */
static void on_speed_down(led_t *s)
{
	if (s->period_ms < 1950)
	{
		s->period_ms = (u16)(s->period_ms + 50);
	}
}

/* K3: 暂停/恢复；暂停时保持 last_tick 更新以避免恢复后突跳 */
static void on_toggle_run(led_t *s)
{
	s->running = (u8)(!s->running);
	s->acc_ms = 0;
}

/*
 * LED 语义事件 -> 回调 的绑定表。
 * - 一条 evt_id 对应一个回调；
 * - 如需“一个事件多个监听者”，可以在表里写多条同 id 的绑定。
 *
 * 放到 code 段：减少 DATA 占用。
 */
static const led_evt_binding_t code s_led_handlers[] = {
	{LED_EVT_TOGGLE_DIR, on_toggle_dir},
	{LED_EVT_SPEED_UP, on_speed_up},
	{LED_EVT_SPEED_DOWN, on_speed_down},
	{LED_EVT_TOGGLE_RUN, on_toggle_run},
};

/*
 * LED 输出回调（注入给 led_obj）：把 logical_mask 写到 P2。
 *
 * 输入：
 * - logical_mask: bit i = 1 表示第 i 个 LED “应该亮”
 *
 * 当前硬件约定：
 * - P2.0~P2.7 接 8 个 LED
 * - 低电平点亮（active-low）
 * 因此：P2 = ~logical_mask
 */
static void led_write_mask_p2(u8 logical_mask)
{
	u8 p2_val;

	/*
	 * logical_mask: bit i=1 表示第 i 个灯亮。
	 * 硬件为低电平点亮，所以需要取反后写到 P2。
	 */
	p2_val = (u8)(~logical_mask);
	P2 = p2_val;
}

/*
 * LED 应用层初始化。
 *
 * 这里做三件事（对应 LVGL 的“驱动注入”思想）：
 * 1) 初始化对象默认状态（周期/方向/running 等）
 * 2) 注入时钟源：now_ms()（本工程用 os_tick_now）
 * 3) 注入输出与事件系统：write_mask / handlers
 */
void app_led_init(void)
{
	/* P2 低电平点亮：上电先全置 1（全灭）。 */
	P2 = 0xFF;

	led_init(&g_led);

	/* 注入时钟源：让框架内部用 now_ms() 自己算 dt。 */
	led_bind_clock(&g_led, os_tick_now);
	/* 注入输出：框架只生成 logical_mask，最终怎么写硬件由这里决定。 */
	led_bind_output(&g_led, led_write_mask_p2);
	
	/* 注入事件回调绑定表：LED_EVT_* -> callback */
	led_bind_event_handlers(&g_led, s_led_handlers, (u8)(sizeof(s_led_handlers) / sizeof(s_led_handlers[0])));
}

/*
 * LED 框架任务（建议 10ms 调度一次）。
 *
 * 任务职责：
 * 1) 消费按键队列：将按键消息喂给 led 对象（触发语义事件分发）
 * 2) 调用 led_handler：根据 now_ms() 推进对象并输出到硬件
 *
 * 注意：
 * - led_handler 内部自算 dt，因此即使本任务偶尔抖动，也能保持语义正确；
 * - 本任务里不做 delay，不阻塞。
 */
void app_led_task(void)
{
	key_msg_t msg;
	u8 evt_id;

	/* 1) 读空按键队列：把所有输入消息喂给 LED 对象（事件分发在框架里做）。 */
	while (os_queue_recv(&g_key_queue, &msg))
	{
		evt_id = app_led_map_from_key(&msg);
		if (evt_id != LED_EVT_NONE)
		{
			led_feed_evt(&g_led, evt_id);
		}
	}

	/* 2) 推进 LED 对象：根据 now_ms() 的 dt 自动步进，必要时输出到硬件。 */
	led_handler(&g_led);
}
