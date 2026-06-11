#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "usbd_def.h"
#include <string.h>
#include "nexlink_rampool.h"
#include "cmsis_compiler.h"

#define TX_QUEUE_SIZE (BUF_COUNT + SMALL_BUF_COUNT)

static tx_item_t tx_queue[TX_QUEUE_SIZE];
static volatile uint8_t tx_head;
static volatile uint8_t tx_tail;

void nexlink_tx_init(void)
{
    tx_head = 0;
    tx_tail = 0;
}

static inline uint8_t in_isr(void)
{
    return (__get_IPSR() != 0);
}

bool nexlink_tx_send(const void *buf, uint16_t len)
{
    if (len > BUF_SIZE)
        return false;

    uint8_t next = (uint8_t)((tx_head + 1) % TX_QUEUE_SIZE);
    if (next == tx_tail)
        return false;

    tx_item_t *item = &tx_queue[tx_head];
    item->len = len;
    item->buf = (uint8_t *)buf;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    tx_head = next;
    __set_PRIMASK(primask);

    return true;
}

void nexlink_tx_send_isr(const void *buf, uint16_t len)
{
    if (len > BUF_SIZE)
        return;

    uint8_t next = (uint8_t)((tx_head + 1) % TX_QUEUE_SIZE);
    if (next == tx_tail)
        return;

    tx_queue[tx_head].len = len;
    tx_queue[tx_head].buf = (uint8_t *)buf;
    tx_head = next;
}

void nexlink_tx_auto(const void *buf, uint16_t len)
{
    if (in_isr())
        nexlink_tx_send_isr(buf, len);
    else
        nexlink_tx_send(buf, len);
}

bool nexlink_tx_receive_nb(tx_item_t *item)
{
    if (tx_tail == tx_head)
        return false;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    *item = tx_queue[tx_tail];
    tx_tail = (uint8_t)((tx_tail + 1) % TX_QUEUE_SIZE);
    __set_PRIMASK(primask);

    return true;
}
