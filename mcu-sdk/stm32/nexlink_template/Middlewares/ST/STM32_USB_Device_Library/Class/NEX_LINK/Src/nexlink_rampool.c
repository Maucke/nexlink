#include "nexlink_rampool.h"
#include <string.h>

static uint8_t buf_pool[BUF_COUNT][BUF_SIZE];
static uint8_t buf_used[BUF_COUNT] = {0};

uint8_t *buf_alloc(void)
{
    for (int i = 0; i < BUF_COUNT; i++)
    {
        if (!buf_used[i])
        {
            buf_used[i] = 1;
            return buf_pool[i];
        }
    }
    return NULL;
}

void buf_free(uint8_t *buf)
{
    for (int i = 0; i < BUF_COUNT; i++)
    {
        if (buf_pool[i] == buf)
        {
            buf_used[i] = 0;
            return;
        }
    }
}
