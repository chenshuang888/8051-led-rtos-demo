#ifndef __KEY_OBJ_H__
#define __KEY_OBJ_H__

#include "types.h"

/*
 * key_obj.h
 *
 * 按键“框架层”（对象 + 状态机）：
 * - 不接触 P3/寄存器；不依赖 OS 队列
 * - 输入：每个 key 的 pressed(1/0)
 * - 输出：在“稳定释放确认”时产生 CLICK/LONG（只输出 action）
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

typedef enum
{
	KEY_ACT_CLICK = 1,
	KEY_ACT_LONG = 2
} key_action_t;

/*
 * 按键输入源读取回调：返回 pressed_mask
 * - bit i = 1 表示第 i 个按键当前按下（已完成 active-low 归一化）
 *
 * 说明：用无参回调是为了更 C51 友好，避免函数指针间接调用参数过多导致编译器报错。
 */
typedef u8 (code *key_read_mask_fn_t)(void);

typedef struct
{
	u8 state[KEY_OBJ_NUM];
	u8 hold_ticks[KEY_OBJ_NUM];
	key_read_mask_fn_t read_mask;
} key_obj_t;

void key_obj_init(key_obj_t *o);

/* 注册输入源读取回调（一般在 app_key_init() 时调用一次即可）。 */
void key_obj_attach_reader(key_obj_t *o, key_read_mask_fn_t fn);

/*
 * 更新某个按键的状态机。
 * - pressed: 1=按下，0=松开
 * - out_action: 若产生事件则填充 out_action 并返回 1；否则返回 0
 */
bit key_obj_update(key_obj_t *o, u8 key_id, u8 pressed, u8 *out_action);

/*
 * 框架轮询：内部调用 read_mask 读输入源，扫描 0..KEY_OBJ_NUM-1
 *
 * 返回：
 * - 1：产出一个事件（写 out_key_id/out_action）
 * - 0：无事件
 *
 * 说明：一次 poll 最多吐出一个事件；若你希望把同周期的事件读空，可在上层 while() 调用。
 */
bit key_obj_poll(key_obj_t *o, u8 *out_key_id, u8 *out_action);

#endif
