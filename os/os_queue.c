#include "os_queue.h"

/*
 * os_queue.c
 *
 * 实现细节：
 * - 自己写了一个字节 memcpy，避免引入更大的库代码。
 * - send/recv 在修改 head/tail/count 时短暂关闭总中断（EA=0），
 *   保证即使 ISR/主循环同时访问也不会把队列状态弄乱。
 *
 * 取舍：
 * - 关中断的时间只覆盖“复制 + 更新索引”的极小片段。
 * - 对常见小消息（1~4B）来说，这个代价在 8051 上一般可接受。
 */

static void os_memcpy(u8 *dst, const u8 *src, u8 n)
{
	while (n--)
	{
		*dst++ = *src++;
	}
}

/*
 * 队列初始化：绑定 storage，并清空 head/tail/count。
 *
 * 参数：
 * - storage: 缓冲区起始地址（由调用者分配）
 * - item_size: 单个元素字节数
 * - length: 队列最大元素个数
 */
void os_queue_init(os_queue_t *q, void *storage, u8 item_size, u8 length)
{
	/* storage 由调用者提供；队列只记录指针与几何信息。 */
	q->buf = (u8 *)storage;
	q->item_size = item_size;
	q->length = length;
	q->head = 0;
	q->tail = 0;
	q->count = 0;
	q->overflow = 0;
}

/*
 * 入队（非阻塞）。
 *
 * 返回：
 * - 1：成功入队
 * - 0：队列满，入队失败（并置 overflow=1）
 */
bit os_queue_send(os_queue_t *q, const void *item)
{
	bit ea = EA;
	u8 *dst;

	EA = 0;

	if (q->count >= q->length)
	{
		/* 记录溢出：提示应用层有消息丢失（队列满）。 */
		q->overflow = 1;
		EA = ea;
		return 0;
	}

	dst = q->buf + (u8)(q->tail * q->item_size);
	os_memcpy(dst, (const u8 *)item, q->item_size);

	q->tail = (u8)((q->tail + 1) % q->length);
	q->count++;

	EA = ea;
	return 1;
}

/*
 * 出队（非阻塞）。
 *
 * 返回：
 * - 1：成功出队并复制到 item
 * - 0：队列空，出队失败
 */
bit os_queue_recv(os_queue_t *q, void *item)
{
	bit ea = EA;
	u8 *src;

	EA = 0;

	if (q->count == 0)
	{
		EA = ea;
		return 0;
	}

	src = q->buf + (u8)(q->head * q->item_size);
	os_memcpy((u8 *)item, src, q->item_size);

	q->head = (u8)((q->head + 1) % q->length);
	q->count--;

	EA = ea;
	return 1;
}

u8 os_queue_count(os_queue_t *q)
{
	/* 返回当前队列中已有元素个数（无锁读，适合调试/统计）。 */
	return q->count;
}

bit os_queue_overflow(os_queue_t *q)
{
	/* overflow=1 表示曾经发生过入队失败（队列满导致丢消息）。 */
	return (q->overflow != 0);
}

void os_queue_overflow_clear(os_queue_t *q)
{
	/* 清除 overflow 标志（需要短暂关中断，避免与 send 同时写）。 */
	bit ea = EA;
	EA = 0;
	q->overflow = 0;
	EA = ea;
}
