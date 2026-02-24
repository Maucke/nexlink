#include "nexlink_ringbuf.h"
#include "FreeRTOS.h"
#include "task.h"

#define RB_SIZE (1024 * 4)

static uint8_t  rb_buf[RB_SIZE];
static uint16_t rb_head;
static uint16_t rb_tail;

void ringbuf_init(void)
{
    rb_head = rb_tail = 0;
}

static uint16_t rb_next(uint16_t v)
{
    return (uint16_t)((v + 1) % RB_SIZE);
}

uint16_t ringbuf_available(void)
{
    if (rb_head >= rb_tail)
        return rb_head - rb_tail;
    return RB_SIZE - rb_tail + rb_head;
}

bool ringbuf_write_from_isr(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        uint16_t next = rb_next(rb_head);
        if (next == rb_tail)
            return false; /* overflow */

        rb_buf[rb_head] = data[i];
        rb_head = next;
    }
    return true;
}

uint16_t ringbuf_read(uint8_t *out, uint16_t maxlen)
{
    uint16_t cnt = 0;

    taskENTER_CRITICAL();
    while (rb_tail != rb_head && cnt < maxlen)
    {
        out[cnt++] = rb_buf[rb_tail];
        rb_tail = rb_next(rb_tail);
    }
    taskEXIT_CRITICAL();

    return cnt;
}
