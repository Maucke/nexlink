#include "nexlink_tx.h"
#include "nexlink_ringbuf.h"
#include "nexlink_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tinyusb.h"

QueueHandle_t txq;

void nexlink_tx_init(void)
{
    txq = xQueueCreate(TX_QUEUE_LEN, sizeof(tx_item_t));
}

void nexlink_tx_send(const void *buf, uint16_t len)
{
    if (len > TX_MAX_LEN)
        return;

    tx_item_t item;
    item.len = len;
    memcpy(item.buf, buf, len);

    xQueueSend(txq, &item, 0);
}

void usb_tx(const void *buf, uint16_t len)
{
    tud_vendor_write(buf, len);
    tud_vendor_flush();
}
