#ifndef __KEY_TYPES_H__
#define __KEY_TYPES_H__

#include "types.h"

/*
 * key_types.h
 *
 * 按键相关的“公共类型”定义（与硬件 / OS 队列无关）：
 * - key_action_t: CLICK / LONG
 * - key_msg_t: {key_id, action}
 *
 * 说明：
 * - 之所以单独抽出来，是为了让 key_obj（框架层）不需要 include app_key.h/os_queue.h；
 * - app_key（应用层）与 app_led（应用层）都可以直接复用同一份 key_msg_t。
 */

typedef enum
{
	KEY_ACT_CLICK = 1,
	KEY_ACT_LONG = 2
} key_action_t;

typedef struct
{
	u8 key_id;  /* 0..3 => P3.0..P3.3 */
	u8 action;  /* key_action_t */
} key_msg_t;

#endif

