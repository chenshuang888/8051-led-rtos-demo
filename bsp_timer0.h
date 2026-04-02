#ifndef __BSP_TIMER0_H__
#define __BSP_TIMER0_H__

#include "types.h"

/*
 * bsp_timer0.h
 *
 * Timer0 驱动：负责“造时间”，提供系统时基（OS tick）。
 *
 * 设计约定：
 * - tick 变量由 timer 模块持有；
 * - 其它模块统一通过 `os_tick_now()` 读取当前时间（ms）。
 *
 * 为什么 8051 读取 tick 需要“原子性”：
 * - tick 是 16 位，在定时器中断里自增；
 * - 8051 读 16 位变量会拆成两次 8 位读；
 * - 如果刚好读完低字节，中断来了把 tick++，再回来读高字节，
 *   就可能得到“撕裂值”（高字节旧、低字节新），造成极难复现的时间 bug。
 */

/* 初始化 Timer0：产生 OS tick 中断（默认 1ms 一次）。 */
void bsp_timer0_init(void);

/* 读取系统 tick（ms）。tick 变量由 timer 模块持有。 */
u16 os_tick_now(void);

#endif
