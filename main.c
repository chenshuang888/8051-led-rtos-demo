#include <REGX52.H>

#include "os.h"
#include "bsp_timer0.h"
#include "app_key.h"

void app_led_task(void);
void app_led_init(void);

/*
 * main.c
 *
 * 系统入口：
 * - 初始化 OS（协作式调度器）
 * - 初始化按键与 LED 应用层
 * - 初始化 Timer0 产生 1ms tick
 * - while(1) 里循环调用 os_run_once() 执行到点任务
 *
 * 注意：
 * - 本工程是协作式调度，所有任务都在主循环里运行，不要在任务里写死等/长延时。
 */

void main(void)
{
	/* P2 低电平点亮：上电先全置 1（全灭）。 */
	P2 = 0xFF;

	os_init();

	app_key_init();
	app_led_init();

	/* LED 框架任务：10ms 调度一次；真正的步进周期由 g_led.period_ms 控制。 */
	(void)os_task_create(app_led_task, 10);

	/* 按键扫描任务：10ms 调度一次；内部状态机去抖。 */
	(void)os_task_create(app_key_task, 10);

	bsp_timer0_init();

	while (1)
	{
		/* 协作式调度：不停跑一轮“到点就执行”的扫描。 */
		os_run_once();
	}
}
