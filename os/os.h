#ifndef __OS_H__
#define __OS_H__

#include "types.h"

/*
 * os.h
 *
 * 一个极简的“协作式”调度器（非抢占、无独立栈）。
 *
 * 使用方式：
 * - 上电后先调用 `os_init()`。
 * - 用 `os_task_create(fn, period_ms)` 注册若干周期任务。
 * - 在 `main()` 的 while(1) 里不停调用 `os_run_once()`。
 *
 * 运行模型：
 * - 依赖全局毫秒 tick（由 bsp_timer0.c 产生）。
 * - 当任务“到点”后在主循环里直接调用任务函数。
 *
 * 注意：
 * - 这是协作式调度：任务函数必须尽量短小、不可阻塞；
 *   否则会拖慢其它任务。
 */

typedef void (*os_task_fn_t)(void);

/* 初始化调度器：清空任务表。 */
void os_init(void);

/* 创建一个周期任务：period = rate_ms。成功返回任务 id，失败返回 0xFF。 */
u8   os_task_create(os_task_fn_t fn, u16 rate_ms);

/* 运行一轮调度：扫描并执行到点任务。 */
void os_run_once(void);

#endif
