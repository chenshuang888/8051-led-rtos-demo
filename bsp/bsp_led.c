#include <REGX52.H>

#include "bsp_led.h"

void bsp_led_init(void)
{
	/* P2 低电平点亮：上电先全置 1（全灭）。 */
	P2 = 0xFF;
}

void bsp_led_write_mask(u8 logical_mask)
{
	u8 p2_val;

	/*
	 * logical_mask: bit i=1 表示第 i 个灯亮。
	 * 硬件为低电平点亮，所以需要取反后写到 P2。
	 */
	p2_val = (u8)(~logical_mask);
	P2 = p2_val;
}

