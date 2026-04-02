#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 * config.h
 *
 * 工程级配置项（频率、tick、任务槽数量等）。
 *
 * 本工程面向经典 8051（如 AT89C51），其内部 RAM 只有 128B，
 * 所以所有“会线性占用 RAM 的东西”都尽量做成可配置。
 *
 * Note: This value should match your actual crystal frequency.
 * The current Keil project file (Project.uvproj) indicates CLOCK(24000000).
 */
#define FOSC_HZ     24000000UL

/* OS tick frequency. 1000 Hz => 1ms tick. */
#define OS_TICK_HZ  1000U

/*
 * Classic 8051 runs timers at FOSC/12 (12T core).
 * If you are using a 1T/6T enhanced core, adjust this divider accordingly.
 *
 * Timer0 的计数值计算：
 *   counts = (FOSC_HZ / TIMER_CLK_DIV) / OS_TICK_HZ
 */
#define TIMER_CLK_DIV 12UL

/*
 * Cooperative scheduler task slots.
 * Keep this small on classic 8051 (only 128 bytes internal RAM).
 * If you are on an 8052/STC with more RAM, you can increase it.
 *
 * 每个任务槽在 `os.c` 里会保存：
 * - 任务函数指针 fn
 * - 周期 rate_ms
 * - 上次运行时间 last_run
 * 因此 OS_MAX_TASKS 会线性占用 RAM（槽越多，RAM 越大）。
 */
#define OS_MAX_TASKS  4U

#endif
