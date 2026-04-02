#include "key_obj.h"

/*
 * 内部状态（保持最小集合）：
 * - UP   : 稳定释放
 * - FALL : 按下确认（第二次采样仍按下才算）
 * - DOWN : 稳定按下（累计 hold_ticks）
 * - RISE : 释放确认（第二次采样仍释放才输出事件）
 */
typedef enum
{
	KEY_ST_UP = 0,
	KEY_ST_FALL = 1,
	KEY_ST_DOWN = 2,
	KEY_ST_RISE = 3
} key_state_t;

void key_obj_init(key_obj_t *o)
{
	u8 i;

	if (o == 0)
	{
		return;
	}

	for (i = 0; i < KEY_OBJ_NUM; i++)
	{
		o->state[i] = KEY_ST_UP;
		o->hold_ticks[i] = 0;
	}
}

bit key_obj_update(key_obj_t *o, u8 key_id, u8 pressed, u8 *out_action)
{
	if (o == 0 || out_action == 0)
	{
		return 0;
	}
	if (key_id >= KEY_OBJ_NUM)
	{
		return 0;
	}

	switch (o->state[key_id])
	{
	case KEY_ST_UP:
		if (pressed)
		{
			o->state[key_id] = KEY_ST_FALL;
		}
		break;

	case KEY_ST_FALL:
		if (pressed)
		{
			o->state[key_id] = KEY_ST_DOWN;
			o->hold_ticks[key_id] = 0;
		}
		else
		{
			o->state[key_id] = KEY_ST_UP;
		}
		break;

	case KEY_ST_DOWN:
		/* 稳定按下：累计按住时长（饱和到 0xFF）。 */
		if (o->hold_ticks[key_id] != 0xFF)
		{
			o->hold_ticks[key_id]++;
		}

		if (!pressed)
		{
			o->state[key_id] = KEY_ST_RISE;
		}
		break;

	case KEY_ST_RISE:
		if (!pressed)
		{
			/* 稳定释放确认：输出一次事件。 */
			o->state[key_id] = KEY_ST_UP;

			if (o->hold_ticks[key_id] >= (u8)KEY_LONG_TICKS)
			{
				*out_action = (u8)KEY_ACT_LONG;
			}
			else
			{
				*out_action = (u8)KEY_ACT_CLICK;
			}
			return 1;
		}
		else
		{
			/* 释放阶段抖动回按下：回到 DOWN。 */
			o->state[key_id] = KEY_ST_DOWN;
		}
		break;

	default:
		o->state[key_id] = KEY_ST_UP;
		break;
	}

	return 0;
}
