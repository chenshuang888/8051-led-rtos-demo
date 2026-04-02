#ifndef __TYPES_H__
#define __TYPES_H__

/*
 * types.h
 *
 * Keil C51(8051) 工程常用类型定义。
 *
 * 说明：
 * - Keil C51 下的 `int` 通常是 16 位，所以这里用 `unsigned int` 作为 u16。
 * - 把类型统一放在一个头文件里，能减少 include 乱象，保证各模块接口一致。
 */

typedef unsigned char u8;
typedef unsigned int  u16; /* Keil C51: usually 16-bit */

#endif
