#include "app_key.h"
#include "bsp_key.h"
#include "key_obj.h"
#include "os_queue.h"

/*
 * app_key.c
 *
 * 按键应用层（glue）：
 * - BSP：读取 P3.0~P3.3 的原始按下状态（bsp_key_read_mask）
 * - 框架：去抖 + 长按识别（key_obj_update）
 * - 输出：把 key_msg_t {key_id, action} 投递到 g_key_queue（os_queue_send）
 *
 * 说明：
 * - 本模块建议 10ms 调度一次；
 * - key_obj 只在“稳定释放确认”时输出一次事件（CLICK/LONG），避免按住期间重复触发。
 */

/* 队列长度：尽量小（省 RAM）。 */
#define KEY_MSG_Q_LEN  8

static key_obj_t s_key_obj;

static key_msg_t s_storage[KEY_MSG_Q_LEN];
os_queue_t g_key_queue;

static u8 app_key_read_mask(void)
{
	/* bit i = 1 表示 Ki 当前按下（已在 BSP 内做 active-low 反相）。 */
	return bsp_key_read_mask();
}

void app_key_init(void)
{
	/* BSP：配置 P3.0~P3.3 为输入（准双向口写 1）。 */
	bsp_key_init();

	/* 框架：初始化去抖/长按状态机。 */
	key_obj_init(&s_key_obj);
	key_obj_attach_reader(&s_key_obj, app_key_read_mask);

	/* OS：初始化按键消息队列。 */
	os_queue_init(&g_key_queue, s_storage, sizeof(key_msg_t), KEY_MSG_Q_LEN);
}

void app_key_task(void)
{
	u8 key_id;
	u8 action;

	/* 读空本周期内产生的所有事件：一次 poll 最多吐出一个事件。 */
	while (key_obj_poll(&s_key_obj, &key_id, &action))
	{
		key_msg_t msg;
		msg.key_id = key_id;
		msg.action = action;
		(void)os_queue_send(&g_key_queue, &msg);
	}

	/* bit i = 1 表示 Ki 当前按下（已在 BSP 内做 active-low 反相）。 */


		/* 框架层输出事件后，由应用层负责入队。 */

			/* 非阻塞发送：若满则丢弃，overflow 由队列内部记录。 */
}
