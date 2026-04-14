#ifndef __LED_OBJ_H__
#define __LED_OBJ_H__

#include "types.h"

/*
 * led_obj.h
 *
 * “LED 对象”（仿 LVGL 风格）的最小框架：
 * - 输入：应用层把 raw key 映射为语义事件 LED_EVT_*
 * - 分发：框架遍历 handlers 表，把语义事件分发给回调
 * - 驱动：应用层注入 write_mask()，框架只生成 logical_mask（bit i=1 表示第 i 盏灯应亮）
 * - 时基：应用层注入 now_ms()（一般绑定 os_tick_now），框架内部自算 dt 并推进流水灯
 *
 * 8051/Keil C51 注意：
 * - struct 里不要用 bit 成员（可能触发 C150），统一用 u8；
 * - 通过函数指针间接调用时参数尽量少（否则可能触发 C212）。
 */

typedef u16 (*led_now_ms_fn_t)(void);
typedef void (*led_write_mask_fn_t)(u8 logical_mask);

typedef enum
{
	LED_EVT_NONE = 0,
	LED_EVT_TOGGLE_DIR = 1,
	LED_EVT_SPEED_UP = 2,
	LED_EVT_SPEED_DOWN = 3,
	LED_EVT_TOGGLE_RUN = 4,
	LED_EVT_TOGGLE_MODE = 5
} led_evt_id_t;

struct led;
typedef void (*led_evt_cb_t)(struct led *s);

typedef struct
{
	u8 id;            /* led_evt_id_t */
	led_evt_cb_t cb;  /* callback */
} led_evt_binding_t;

typedef struct led
{
	/* “模型”状态（由事件回调修改） */
	u8 idx;        /* 单灯模式：0..7；双灯模式：0..6（表示左灯） */
	u8 dir;        /* 0: 正向，1: 反向 */
	u8 running;    /* 1: 流动，0: 暂停 */
	u8 pair_mode;  /* 0: 单灯流水；1: 相邻双灯流水 */
	u16 period_ms; /* 步进周期（ms） */

	/* “运行时”状态（由 handler 维护） */
	u16 acc_ms;
	u16 last_tick;
	led_now_ms_fn_t now_ms;

	/* 输出（硬件绑定），类似 LVGL 的 flush_cb */
	led_write_mask_fn_t write_mask;

	/* 事件绑定表（语义 -> 回调），建议放 code 段节省 DATA */
	const led_evt_binding_t code *evt_bindings;
	u8 evt_binding_count;
} led_t;

void led_init(led_t *s);
void led_bind_clock(led_t *s, led_now_ms_fn_t now_ms);
void led_bind_output(led_t *s, led_write_mask_fn_t write_mask);
void led_bind_event_handlers(led_t *s, const led_evt_binding_t code *bindings, u8 count);
void led_feed_evt(led_t *s, u8 evt_id);
void led_handler(led_t *s);

#endif

