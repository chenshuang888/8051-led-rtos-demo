#ifndef __LED_OBJ_H__
#define __LED_OBJ_H__

#include "types.h"
#include "app_key.h" /* key_msg_t / KEY_ACT_* */

/*
 * led_obj.h
 *
 * “LED 对象”（仿 LVGL 风格）的最小框架：
 * - 输入：按键模块产生的 key_msg_t（key_id + action）
 * - 映射：应用层提供 map_key()，把 raw key 映射成语义事件 LED_EVT_*
 * - 分发：框架遍历 handlers 表，把语义事件分发给回调
 * - 驱动：应用层注入 write_mask()，框架只生成“逻辑灯状态”logical_mask
 * - 时基：应用层注入 now_ms()，框架内部算 dt 并推进流水灯
 *
 * 8051/Keil C51 注意：
 * - struct 里不能用 bit 成员（会触发 C150），这里全部用 u8 代替。
 * - 通过函数指针的间接调用参数要尽量少（否则可能触发 C212）。
 */

typedef u16 (*led_now_ms_fn_t)(void);
typedef void (*led_write_mask_fn_t)(u8 logical_mask);

typedef enum
{
        LED_EVT_NONE = 0,
        LED_EVT_TOGGLE_DIR = 1,
        LED_EVT_SPEED_UP = 2,
        LED_EVT_SPEED_DOWN = 3,
        LED_EVT_TOGGLE_RUN = 4
} led_evt_id_t;

/* key_id/action -> semantic event id */
typedef u8 (*led_key_map_fn_t)(u8 key_id, u8 action);

typedef void (*led_evt_cb_t)(struct led *s);

typedef struct
{
        u8 id;            /* led_evt_id_t */
        led_evt_cb_t cb; /* callback */
} led_evt_binding_t;

typedef struct led
{
        /* “模型”状态（由事件回调修改） */
        u8 idx;                 /* 0..7 */
        u8 dir;                 /* 0: 正向, 1: 反向 */
        u8 running;             /* 1: 流动, 0: 暂停 */
        u16 period_ms;  /* 步进周期（ms） */

        /* “运行时”状态（由 handler 维护） */
        u16 acc_ms;
        u16 last_tick;
        led_now_ms_fn_t now_ms;

        /* “驱动”输出（硬件绑定），类似 LVGL 的 flush_cb */
        led_write_mask_fn_t write_mask;

        /* 输入映射（raw -> 语义），由应用层提供 */
        led_key_map_fn_t map_key;

        /* 事件绑定表（语义 -> 回调），由应用层提供 */
        const led_evt_binding_t code *evt_bindings;
        u8 evt_binding_count;
} led_t;

/* 初始化 led 对象：设置默认状态并清空所有注入接口/绑定表。 */
void led_init(led_t *s);

/* 注入时钟源 now_ms()（毫秒）：一般绑定为 os_tick_now。 */
void led_bind_clock(led_t *s, led_now_ms_fn_t now_ms);

/* 注入输出回调 write_mask()：框架输出 logical_mask，硬件细节由回调决定。 */
void led_bind_output(led_t *s, led_write_mask_fn_t write_mask);

/* 注入 raw key 映射函数：key_id/action -> LED_EVT_*。 */
void led_bind_key_mapper(led_t *s, led_key_map_fn_t map_key);

/* 注入语义事件处理表：LED_EVT_* -> callback（建议放 code 段）。 */
void led_bind_event_handlers(led_t *s, const led_evt_binding_t code *bindings, u8 count);

/* 喂入按键消息：内部做 raw->semantic->dispatch。 */
void led_feed_key(led_t *s, const key_msg_t *msg);

/* 主处理函数：内部用 now_ms() 自算 dt，推进 acc_ms，到点则输出一步。 */
void led_handler(led_t *s);

#endif
