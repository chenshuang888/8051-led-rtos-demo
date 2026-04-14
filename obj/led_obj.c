#include "led_obj.h"

/*
 * led_obj.c
 *
 * LED“对象框架”实现：
 * - 事件回调只修改对象状态（方向/速度/运行/模式等）
 * - led_handler() 根据 now_ms() 推进时间，到点则 step 一次并通过 write_mask() 输出
 *
 * 输出采用 logical_mask：bit i=1 表示第 i 盏灯应亮。
 * 具体如何映射到 P2、是否反相等由应用层注入的 write_mask() 决定。
 */

static void led_render(led_t *s)
{
	u8 logical_mask;

	if (s == 0 || s->write_mask == 0)
	{
		return;
	}

	if (s->pair_mode)
	{
		/*
		 * 相邻双灯模式：
		 * - idx 表示“左灯”位置：0..6
		 * - 序列：(0,1)->(1,2)->...->(6,7)->回到(0,1)
		 */
		u8 left = s->idx;
		if (left >= 7)
		{
			left = 0;
		}
		logical_mask = (u8)((1u << left) | (1u << (left + 1)));
	}
	else
	{
		/* 单灯流水：idx 0..7 循环 */
		logical_mask = (u8)(1u << (s->idx & 0x07));
	}

	s->write_mask(logical_mask);
}

static void led_step(led_t *s)
{
	u8 idx;

	if (s == 0)
	{
		return;
	}

	led_render(s);

	if (!s->pair_mode)
	{
		/* 单灯模式：8 个位置循环 */
		if (s->dir == 0)
		{
			s->idx = (u8)((s->idx + 1) & 0x07);
		}
		else
		{
			s->idx = (u8)((s->idx + 7) & 0x07);
		}
		return;
	}

	/* 双灯模式：idx 作为左灯，范围 0..6 */
	idx = s->idx;
	if (idx >= 7)
	{
		idx = 0;
	}

	if (s->dir == 0)
	{
		idx = (u8)((idx >= 6) ? 0 : (idx + 1));
	}
	else
	{
		idx = (u8)((idx == 0) ? 6 : (idx - 1));
	}

	s->idx = idx;
}

static void led_dispatch_evt(led_t *s, u8 evt_id)
{
	u8 i;

	if (s == 0 || evt_id == LED_EVT_NONE)
	{
		return;
	}
	if (s->evt_bindings == 0 || s->evt_binding_count == 0)
	{
		return;
	}

	/* LVGL-style: allow multiple handlers to listen to the same event id. */
	for (i = 0; i < s->evt_binding_count; i++)
	{
		const led_evt_binding_t code *b = &s->evt_bindings[i];
		if (b->id == evt_id && b->cb)
		{
			b->cb(s);
		}
	}
}

void led_init(led_t *s)
{
	if (s == 0)
	{
		return;
	}

	s->idx = 0;
	s->dir = 0;
	s->running = 1;
	s->pair_mode = 0;
	s->period_ms = 500;

	s->acc_ms = 0;
	s->last_tick = 0;
	s->now_ms = 0;
	s->write_mask = 0;
	s->evt_bindings = 0;
	s->evt_binding_count = 0;
}

void led_bind_clock(led_t *s, led_now_ms_fn_t now_ms)
{
	if (s == 0)
	{
		return;
	}

	s->now_ms = now_ms;
	s->last_tick = now_ms ? now_ms() : 0;
}

void led_bind_output(led_t *s, led_write_mask_fn_t write_mask)
{
	if (s == 0)
	{
		return;
	}

	s->write_mask = write_mask;

	/* Clear output on bind. */
	if (write_mask)
	{
		write_mask(0);
	}
}

void led_bind_event_handlers(led_t *s, const led_evt_binding_t code *bindings, u8 count)
{
	if (s == 0)
	{
		return;
	}
	s->evt_bindings = bindings;
	s->evt_binding_count = count;
}

void led_feed_evt(led_t *s, u8 evt_id)
{
	led_dispatch_evt(s, evt_id);
}

void led_handler(led_t *s)
{
	u16 now;
	u16 dt;

	if (s == 0 || s->now_ms == 0)
	{
		return;
	}

	now = s->now_ms();
	dt = (u16)(now - s->last_tick); /* 无符号回绕安全 */
	s->last_tick = now;

	/* 暂停时也要更新 last_tick（上面已做），避免恢复后 dt 过大 */
	if (!s->running)
	{
		return;
	}

	s->acc_ms = (u16)(s->acc_ms + dt);
	if (s->acc_ms >= s->period_ms)
	{
		s->acc_ms = 0;
		led_step(s);
	}
}

