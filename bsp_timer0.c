#include <REGX52.H>
#include "config.h"
#include "types.h"
#include "bsp_timer0.h"

/*
 * bsp_timer0.c
 *
 * 实现要点：
 * - Timer0 使用模式 1（16 位定时器）。
 * - 每次进中断都重装 TH0/TL0，保证 tick 周期稳定。
 * - 在 ISR 中 `g_os_tick++`，形成全局毫秒时基。
 */

static unsigned char g_t0_th = 0;
static unsigned char g_t0_tl = 0;
static volatile u16 g_os_tick = 0;

/*
 * 获取当前系统 tick（ms）。
 *
 * 为什么要关中断：
 * - g_os_tick 是 16 位
 * - 8051 读 16 位会拆成两次 8 位操作
 * - 关中断可避免“撕裂读”
 */
u16 os_tick_now(void)
{
    u16 t;
    bit ea = EA;

    /* 读取 16 位 tick 需要关中断，避免 8051 读两字节时被中断打断导致撕裂。 */
    EA = 0;
    t = g_os_tick;
    EA = ea;
    return t;
}

/*
 * 重装 Timer0 的 TH0/TL0。
 *
 * 说明：
 * - 本工程使用“每次中断重装”的方式保证 tick 周期稳定。
 */
static void t0_reload(void)
{
    /* 重装定时器：保证下一次溢出仍然是 1 个 tick 的间隔。 */
    TH0 = g_t0_th;
    TL0 = g_t0_tl;
}

/*
 * Timer0 初始化：配置为按 OS_TICK_HZ 产生中断。
 *
 * 计算过程：
 * - counts = (FOSC_HZ / TIMER_CLK_DIV) / OS_TICK_HZ
 * - reload = 65536 - counts
 * - TH0/TL0 = reload
 */
void bsp_timer0_init(void)
{
    unsigned long counts;
    unsigned int reload;

    /* 定时器计数时钟 = FOSC / TIMER_CLK_DIV */
    counts = (FOSC_HZ / TIMER_CLK_DIV) / OS_TICK_HZ;

    /* 16 位定时器：reload = 65536 - counts */
    reload = (unsigned int)(65536UL - (unsigned long)counts);

    g_t0_th = (unsigned char)(reload >> 8);
    g_t0_tl = (unsigned char)(reload & 0xFF);

    TMOD &= 0xF0;   /* 清除 T0 配置位 */
    TMOD |= 0x01;   /* 模式 1：16 位定时器 */

    t0_reload();

    TF0 = 0;
    ET0 = 1;
    EA  = 1;
    TR0 = 1;
}

void timer0_isr(void) interrupt 1
{
    /*
     * Timer0 中断服务：
     * - 重装 TH0/TL0
     * - tick++
     *
     * 注意：ISR 要尽量短小，避免影响主循环任务执行。
     */
    t0_reload();
    g_os_tick++;
}
