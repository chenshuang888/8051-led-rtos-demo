#ifndef __BSP_LED_H__
#define __BSP_LED_H__

#include "types.h"

/*
 * bsp_led.h
 *
 * LED 硬件驱动层（BSP）：
 * - 统一在这里操作 P2，应用层/框架层不直接写 P2
 * - 对上层提供“逻辑掩码”接口：bit i = 1 表示第 i 个灯应亮
 *
 * 默认硬件约定：
 * - 8 个 LED 接在 P2.0~P2.7
 * - 低电平点亮（active-low），因此写入 P2 的值为 ~logical_mask
 */

/* 初始化 LED 端口（上电全灭）。 */
void bsp_led_init(void);

/* 写入逻辑掩码到硬件端口。 */
void bsp_led_write_mask(u8 logical_mask);

#endif
