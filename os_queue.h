#ifndef __OS_QUEUE_H__
#define __OS_QUEUE_H__

#include <REGX52.H>
#include "types.h"

/*
 * os_queue.h
 *
 * 一个很小的环形队列（ring buffer），给 8051 用：
 * - 定长队列、定长元素（item_size 固定）。
 * - 发送/接收都是非阻塞：成功返回 1，失败返回 0。
 * - 为了在中断/主循环同时访问时不乱，关键区短暂关总中断（EA=0）。
 *
 * 重要说明：
 * - 这不是完整 RTOS 队列：没有阻塞等待、没有超时。
 * - 队列长度要尽量小，否则 128B RAM 直接爆。
 */

typedef struct
{
	u8 *buf;		   /* 缓冲区指针（按字节寻址） */
	u8 item_size;	   /* 单个元素大小（字节） */
	u8 length;		   /* 队列长度（元素个数） */
	volatile u8 head;
	volatile u8 tail;
	volatile u8 count;
	volatile u8 overflow;
} os_queue_t;

/* storage 必须提供：length * item_size 字节 */
void os_queue_init(os_queue_t *q, void *storage, u8 item_size, u8 length);

/* 非阻塞 API：成功返回 1，失败（满/空）返回 0。 */
bit os_queue_send(os_queue_t *q, const void *item);
bit os_queue_recv(os_queue_t *q, void *item);

/* 可选辅助函数：用于调试/统计。 */
/* 返回当前队列元素个数。 */
u8 os_queue_count(os_queue_t *q);
/* 返回 overflow 标志（1 表示曾经丢过消息）。 */
bit os_queue_overflow(os_queue_t *q);
/* 清除 overflow 标志。 */
void os_queue_overflow_clear(os_queue_t *q);

#endif
