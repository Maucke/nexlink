#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usbd_def.h"
#include <string.h>
#include "nexlink_rampool.h"

static QueueHandle_t txq;

void nexlink_tx_init(void)
{
    txq = xQueueCreate(BUF_COUNT + SMALL_BUF_COUNT, sizeof(tx_item_t));
}

void nexlink_tx_send(const void *buf, uint16_t len)
{
    if (len > BUF_SIZE)
        return;

    tx_item_t item;
    item.len = len;
    item.buf = (uint8_t *)buf;

    xQueueSend(txq, &item, portMAX_DELAY);
}

void nexlink_tx_send_isr(const void *buf, uint16_t len)
{
    BaseType_t hpw = pdFALSE;
    if (len > BUF_SIZE)
        return;

    tx_item_t item;
    item.len = len;
    item.buf = (uint8_t *)buf;

    xQueueSendFromISR(txq, &item, &hpw);
    portYIELD_FROM_ISR(hpw);
}

static inline uint8_t in_isr(void)
{
    return (__get_IPSR() != 0);
}

void nexlink_tx_auto(const void *buf, uint16_t len)
{
    if (in_isr())
        nexlink_tx_send_isr(buf, len);
    else
        nexlink_tx_send(buf, len);
}

void NexLinkTxTask(void *arg)
{
    tx_item_t item;
    for (;;)
    {
			if (xQueueReceive(txq, &item, portMAX_DELAY))
			{
					usb_tx(item.buf, item.len);
			}
    }
}
