#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usbd_def.h"
#include <string.h>

#define TX_MAX_LEN 256
#define TX_QUEUE_LEN 8

typedef struct
{
    uint16_t len;
    uint8_t buf[TX_MAX_LEN];
} tx_item_t;

static QueueHandle_t txq;

void nexlink_tx_init(void)
{
    txq = xQueueCreate(TX_QUEUE_LEN, sizeof(tx_item_t));
}

void nexlink_tx_send(const void *data, uint16_t len)
{
    if (len > TX_MAX_LEN)
        return;

    tx_item_t item;
    item.len = len;
    memcpy(item.buf, data, len);

    xQueueSend(txq, &item, portMAX_DELAY);
}

void NexLinkTxTask(void *arg)
{
    tx_item_t item;
    for (;;)
    {
			if (xQueueReceive(txq, &item, portMAX_DELAY))
					usb_tx(item.buf, item.len);
    }
}
