#include <REGX52.H>

#include "app_key.h"
#include "os_queue.h"

/*
 * app_key.c
 *
 * 按键扫描实现。
 *
 * 去抖方式：
 * - 每个键 4 个状态：UP -> FALL -> DOWN -> RISE -> UP
 * - 按下/释放都需要连续两次采样一致才确认（若任务周期 10ms，则约 20ms 去抖）。
 *
 * 事件生成：
 * - 当确认释放（RISE 确认）时输出事件：
 *   - 按住时间 >= KEY_LONG_TICKS 认为 LONG
 *   - 否则认为 CLICK
 * - 事件通过 `g_key_queue` 输出（非阻塞）。队列满则丢弃，并在队列里记录 overflow。
 */

/* 队列长度：尽量小（省 RAM）。 */
#define KEY_MSG_Q_LEN  8

/* 长按阈值：单位是任务 tick（建议任务周期 10ms）。 */
#define KEY_LONG_TICKS 80 /* 80 * 10ms = 800ms */

/* 去抖状态机（按任务周期做 2 次采样确认）。 */
typedef enum
{
	KEY_ST_UP = 0,
	KEY_ST_FALL = 1,
	KEY_ST_DOWN = 2,
	KEY_ST_RISE = 3
} key_state_t;

static u8 s_state[4];
static u8 s_hold_ticks[4];

static key_msg_t s_storage[KEY_MSG_Q_LEN];
os_queue_t g_key_queue;

/*
 * 读取某个按键的原始电平。
 *
 * 返回：
 * - 1：按下（引脚为 0）
 * - 0：松开（引脚为 1）
 */
static bit key_raw_pressed(u8 idx)
{
	/* 低电平有效：引脚读到 0 视为按下。 */
	switch (idx)
	{
	case 0:
		return (P3_0 == 0);
	case 1:
		return (P3_1 == 0);
	case 2:
		return (P3_2 == 0);
	case 3:
		return (P3_3 == 0);
	default:
		return 0;
	}
}

/*
 * 按键模块初始化：
 * - 配置 P3.0~P3.3 为输入（准双向口写 1）
 * - 初始化状态机与计数
 * - 初始化输出队列 g_key_queue
 */
void app_key_init(void)
{
	u8 i;

	/* 准双向口：写 1 作为输入（内部上拉）。 */
	P3 |= 0x0F;

	for (i = 0; i < 4; i++)
	{
		s_state[i] = KEY_ST_UP;
		s_hold_ticks[i] = 0;
	}

	os_queue_init(&g_key_queue, s_storage, sizeof(key_msg_t), KEY_MSG_Q_LEN);
}

/*
 * 按键扫描任务：建议 10ms 调一次。
 *
 * 每个按键独立状态机：
 * - UP   : 稳定释放
 * - FALL : 按下确认
 * - DOWN : 稳定按下（累计按住时间）
 * - RISE : 释放确认（确认后输出 CLICK/LONG）
 */
void app_key_task(void)
{
	u8 i;

	for (i = 0; i < 4; i++)
	{
		bit raw = key_raw_pressed(i);

		switch (s_state[i])
		{
		case KEY_ST_UP:
			/* 稳定释放态：等待可能的按下。 */
			if (raw)
			{
				s_state[i] = KEY_ST_FALL;
			}
			break;

		case KEY_ST_FALL:
			/* FALL：第二次采样仍为按下，才确认进入 DOWN。 */
			if (raw)
			{
				s_state[i] = KEY_ST_DOWN;
				s_hold_ticks[i] = 0;
			}
			else
			{
				s_state[i] = KEY_ST_UP;
			}
			break;

		case KEY_ST_DOWN:
			/* 稳定按下态：累计按住时间；等待释放。 */
			if (s_hold_ticks[i] != 0xFF)
			{
				s_hold_ticks[i]++;
			}
			if (!raw)
			{
				s_state[i] = KEY_ST_RISE;
			}
			break;

		case KEY_ST_RISE:
			/* RISE：第二次采样仍为释放，才确认并输出事件。 */
			if (!raw)
			{
				key_msg_t msg;
				s_state[i] = KEY_ST_UP;
				msg.key_id = i;
				if (s_hold_ticks[i] >= KEY_LONG_TICKS)
				{
					msg.action = KEY_ACT_LONG;
				}
				else
				{
					msg.action = KEY_ACT_CLICK;
				}
				/* 非阻塞发送：若满则丢弃，overflow 由队列内部记录。 */
				(void)os_queue_send(&g_key_queue, &msg);
			}
			else
			{
				s_state[i] = KEY_ST_DOWN;
			}
			break;

		default:
			s_state[i] = KEY_ST_UP;
			break;
		}
	}
}
