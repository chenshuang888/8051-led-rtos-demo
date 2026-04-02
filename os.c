#include "config.h"
#include "os.h"
#include "bsp_timer0.h"

/*
 * Minimal cooperative scheduler (STM32-template style):
 * - Global tick (ms) updated by timer ISR
 * - Task table: { fn, rate_ms, last_run }
 * - Main loop calls os_run_once() to execute due tasks
 *
 * 时间模型：
 * - 每个任务一个固定周期 `rate_ms`。
 * - 用溢出安全的比较： (u16)(now - last_run) >= rate_ms
 * - 为减少长期漂移，命中后用 last_run += rate_ms，而不是 last_run = now。
 *
 * 约束：
 * - 8051 内部 RAM 很小，OS_MAX_TASKS 要尽量小。
 * - 不做任务删除/挂起等复杂功能，保持 API 极简可控。
 */

typedef struct
{
    os_task_fn_t fn;
    u16 rate_ms;
    u16 last_run;
} os_task_t;

static os_task_t g_tasks[OS_MAX_TASKS];
static u8 g_task_num = 0;

/*
 * 调度器初始化：清空任务表。
 *
 * 调用时机：
 * - 上电初始化阶段调用一次即可。
 */
void os_init(void)
{
    u8 i;
    for (i = 0; i < OS_MAX_TASKS; i++)
    {
        g_tasks[i].fn = 0;
        g_tasks[i].rate_ms = 0;
        g_tasks[i].last_run = 0;
    }
    g_task_num = 0;
}

/*
 * 注册一个周期任务。
 *
 * 参数：
 * - fn: 任务函数指针（不可为 0）
 * - rate_ms: 周期（ms，不可为 0）
 *
 * 返回：
 * - 成功：任务 id（0..OS_MAX_TASKS-1）
 * - 失败：0xFF
 *
 * 注意：
 * - 本调度器不做动态删除，创建后一直存在。
 * - 任务首次会“立即运行一次”，实现方式是 last_run = now - rate。
 */
u8 os_task_create(os_task_fn_t fn, u16 rate_ms)
{
    u16 now;

    if (fn == 0 || rate_ms == 0)
    {
        return 0xFF;
    }

    if (g_task_num >= OS_MAX_TASKS)
    {
        /* 任务槽用完：减少任务数，或提高 OS_MAX_TASKS（代价是更多 RAM）。 */
        return 0xFF;
    }

    now = os_tick_now();

    g_tasks[g_task_num].fn = fn;
    g_tasks[g_task_num].rate_ms = rate_ms;
    /* Make the first run happen immediately (like last_run = now - rate). */
    g_tasks[g_task_num].last_run = (u16)(now - rate_ms);

    g_task_num++;
    return (u8)(g_task_num - 1);
}

/*
 * 执行一轮调度：扫描所有任务，执行到点的任务。
 *
 * 调用位置：
 * - 建议在 while(1) 中尽可能频繁调用。
 *
 * 注意：
 * - 所有任务都在这里串行运行；
 * - 单个任务执行时间过长，会直接影响其它任务的实时性。
 */
void os_run_once(void)
{
    u8 i;
    u16 now = os_tick_now();

    for (i = 0; i < g_task_num; i++)
    {
        os_task_t *t = &g_tasks[i];
        if (t->fn == 0)
        {
            continue;
        }

        /*
         * Overflow-safe time comparison using unsigned subtraction.
         * Keep cadence by accumulating last_run (reduces drift).
         *
         * 例子：
         * - 任务周期 10ms，某次因为别的任务占用导致它在 13ms 才运行；
         *   这里仍然只让 last_run += 10ms，这样下次触发仍尽量贴近“理想节拍”，
         *   不会越跑越飘。
         */
        if ((u16)(now - t->last_run) >= t->rate_ms)
        {
            t->last_run = (u16)(t->last_run + t->rate_ms);
            t->fn();
        }
    }
}
