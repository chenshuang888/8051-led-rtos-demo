#ifndef __APP_LED_H__
#define __APP_LED_H__

/*
 * app_led.h
 *
 * LED 应用层对外接口：
 * - app_led_init(): 初始化 LED 对象框架、注入时钟/输出、绑定事件回调等
 * - app_led_task(): 周期任务（建议 10ms），消费按键队列并驱动 LED handler
 *
 * 说明：
 * - main.c 应该通过 include 本头文件获取函数声明，而不是手写 extern/声明，
 *   这样接口变更时不会出现“声明与实现不一致”的隐蔽问题。
 */

void app_led_init(void);
void app_led_task(void);

#endif
