#include <REGX52.H>

#include "bsp_key.h"

void bsp_key_init(void)
{
	/*
	 * 8051 准双向口做输入的常规写法：先写 1，让内部上拉生效，再读引脚电平。
	 * 这里只设置低 4 位（P3.0~P3.3），避免影响 P3 的其它引脚用途。
	 */
	P3 |= 0x0F;
}

u8 bsp_key_read_mask(void)
{
	u8 raw;
	u8 pressed;

	/* P3.0~P3.3：1=松开，0=按下（active-low） */
	raw = (u8)(P3 & 0x0F);

	/* 对上层统一成：1=按下 */
	pressed = (u8)((~raw) & 0x0F);
	return pressed;
}

