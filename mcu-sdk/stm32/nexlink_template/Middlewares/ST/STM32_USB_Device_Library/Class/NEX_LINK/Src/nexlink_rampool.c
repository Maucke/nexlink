#include "nexlink_rampool.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include "cmsis_compiler.h"

/* -------- large buffer pool -------- */

static uint8_t large_pool[BUF_COUNT][BUF_SIZE];
static uint8_t large_used[BUF_COUNT];

/* -------- small buffer pool -------- */

static uint8_t small_pool[SMALL_BUF_COUNT][SMALL_BUF_SIZE];
static uint8_t small_used[SMALL_BUF_COUNT];

/* -------- cpu atomic -------- */

static inline uint32_t irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void irq_unlock(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/* -------- allocation -------- */

uint8_t *buf_alloc(uint16_t need)
{
    uint32_t key = irq_lock();
    /* try small pool first */
    if (need <= SMALL_BUF_SIZE)
    {
        for (int i = 0; i < SMALL_BUF_COUNT; i++)
        {
            if (!small_used[i])
            {
                small_used[i] = 1;
                irq_unlock(key);
                return small_pool[i];
            }
        }
        /* fallback to large pool */
    }

    for (int i = 0; i < BUF_COUNT; i++)
    {
        if (!large_used[i])
        {
            large_used[i] = 1;
            irq_unlock(key);
            return large_pool[i];
        }
    }
    irq_unlock(key);
    return NULL;
}

/* -------- free -------- */

void buf_free(uint8_t *buf)
{

    uint32_t key = irq_lock();
    for (int i = 0; i < SMALL_BUF_COUNT; i++)
    {
        if (small_pool[i] == buf)
        {
            small_used[i] = 0;
            irq_unlock(key);
            return;
        }
    }

    for (int i = 0; i < BUF_COUNT; i++)
    {
        if (large_pool[i] == buf)
        {
            large_used[i] = 0;
            irq_unlock(key);
            return;
        }
    }
    irq_unlock(key);
}
