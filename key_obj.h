#ifndef __KEY_OBJ_H__
#define __KEY_OBJ_H__

#include "types.h"
#include "key_types.h"

/*
 * key_obj.h
 *
 * 按键“框架层”（对象 + 状态机）：
 * - 不接触 P3/寄存器；不依赖 OS 队列
 * - 输入：每个 key 的 pressed(1/0)
 * - 输出：在“稳定释放确认”时产生 CLICK/LONG 事件
 *
 * 去抖策略（保持简单，和你原来的 app_key.c 一致）：
 * - UP -> FALL -> DOWN -> RISE -> UP
 * - 按下/释放都采用“2 次采样一致确认”
 *
 * 长按判定：
 * - 在 DOWN 状态累计 hold_ticks
 * - 在 RISE 确认释放时输出 CLICK / LONG（只输出一次）
 *
 * 约定：
 * - 本框架默认用于 10ms 调度周期；KEY_LONG_TICKS=80 => 800ms
 */

#define KEY_OBJ_NUM        4
#define KEY_LONG_TICKS     80u /* 80 * 10ms = 800ms */

typedef struct
{
	u8 state[KEY_OBJ_NUM];
	u8 hold_ticks[KEY_OBJ_NUM];
} key_obj_t;

void key_obj_init(key_obj_t *o);

/*
 * 更新某个按键的状态机。
 * - pressed: 1=按下，0=松开
 * - out: 若产生事件则填充 out 并返回 1；否则返回 0
 */
bit key_obj_update(key_obj_t *o, u8 key_id, u8 pressed, key_msg_t *out);

#endif

