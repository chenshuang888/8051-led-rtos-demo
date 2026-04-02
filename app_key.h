#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include "types.h"
#include "os_queue.h"

/*
 * app_key.h
 *
 * 按键扫描模块（8051）。
 *
 * 硬件：
 * - 4 个按键接在 P3.0~P3.3
 * - 默认低电平有效（按下=0）
 *
 * 软件：
 * - 提供周期扫描任务（建议 10ms 调一次）
 * - 用简单状态机做去抖
 * - 将按键事件输出到队列 `g_key_queue`
 *   （仿 FreeRTOS 的句柄风格：对外暴露队列对象供其它任务轮询/消费）
 */

/*
 * 按键在 P3.0~P3.3，默认低电平有效（按下=0）。
 *
 * key_id 映射：
 * - 0: P3.0
 * - 1: P3.1
 * - 2: P3.2
 * - 3: P3.3
 */

/* 按键模块初始化：配置 IO、清空状态机、初始化 g_key_queue。 */
void app_key_init(void);

/* 按键扫描任务：周期调用（建议 10ms）。 */
void app_key_task(void);

/*
 * Key message queue (very small, RAM-friendly).
 * Messages are emitted on stable release only: CLICK or LONG.
 *
 * 说明：
 * - 本模块不输出“按下期间持续触发”的 down 事件；
 * - 它只在“确认释放”时输出一次事件，这样就能带上按压时长信息
 *   （CLICK / LONG）。
 */
typedef enum
{
	KEY_ACT_CLICK = 1,
	KEY_ACT_LONG = 2
} key_action_t;

typedef struct
{
	u8 key_id;  /* 0..3 => P3.0..P3.3 */
	u8 action;  /* key_action_t */
} key_msg_t;

/* Expose the queue handle (FreeRTOS QueueHandle_t style). */
extern os_queue_t g_key_queue;

#endif
