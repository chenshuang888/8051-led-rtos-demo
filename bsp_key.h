#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "types.h"

/*
 * bsp_key.h
 *
 * 按键硬件层（BSP）：
 * - 只负责端口初始化与读取原始状态
 * - 默认硬件：4 个按键在 P3.0~P3.3，低电平有效（按下=0）
 *
 * 输出格式：
 * - bsp_key_read_mask() 返回 4bit 掩码
 * - bit i = 1 表示 Ki 当前“按下”（已在 BSP 内做 active-low 反相）
 */

void bsp_key_init(void);
u8   bsp_key_read_mask(void);

#endif

