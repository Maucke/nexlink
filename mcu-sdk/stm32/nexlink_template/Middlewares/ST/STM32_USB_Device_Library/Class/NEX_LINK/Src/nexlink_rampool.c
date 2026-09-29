#include "nexlink_rampool.h"
#include "nexlink_lock.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include "cmsis_compiler.h"

/* -------- large buffer pool -------- */

static uint8_t large_pool[BUF_COUNT][BUF_SIZE];
static uint8_t large_used[BUF_COUNT];

/* -------- small buffer pool -------- */

static uint8_t small_pool[SMALL_BUF_COUNT][SMALL_BUF_SIZE];
static uint8_t small_used[SMALL_BUF_COUNT];

/*
 * 池标记被多个上下文碰：
 *   - buf_alloc：RX 任务回命令、default 任务发心跳
 *   - buf_free：nexlink_tx_send（队列满回滚）、nexlink_tx_poll（传完回收）
 * 不互斥的话两个上下文可能同时拿到同一块缓冲。锁必须中断/任务通用，
 * 见 nexlink_lock.h。
 */

/* -------- allocation -------- */

uint8_t *buf_alloc(uint16_t need)
{
    uint8_t *ret = NULL;
    uint32_t lock = nexlink_lock_enter();

    /* try small pool first */
    if (need <= SMALL_BUF_SIZE)
    {
        for (int i = 0; i < SMALL_BUF_COUNT; i++)
        {
            if (!small_used[i])
            {
                small_used[i] = 1;
                ret = small_pool[i];
                break;
            }
        }
        /* fallback to large pool */
    }

    if (ret == NULL)
    {
        for (int i = 0; i < BUF_COUNT; i++)
        {
            if (!large_used[i])
            {
                large_used[i] = 1;
                ret = large_pool[i];
                break;
            }
        }
    }

    nexlink_lock_exit(lock);

    return ret;
}

/* -------- free -------- */

void buf_free(uint8_t *buf)
{
    if (buf == NULL)
        return;

    uint32_t lock = nexlink_lock_enter();

    for (int i = 0; i < SMALL_BUF_COUNT; i++)
    {
        if (small_pool[i] == buf)
        {
            small_used[i] = 0;
            goto out;
        }
    }

    for (int i = 0; i < BUF_COUNT; i++)
    {
        if (large_pool[i] == buf)
        {
            large_used[i] = 0;
            goto out;
        }
    }

out:
    nexlink_lock_exit(lock);
}
