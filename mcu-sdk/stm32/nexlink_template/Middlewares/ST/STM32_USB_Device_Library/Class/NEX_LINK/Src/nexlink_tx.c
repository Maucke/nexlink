#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usbd_def.h"
#include <string.h>
#include "usbd_nex_link.h"

static QueueHandle_t txq;

static uint8_t tx_buf_pool[TX_BUF_COUNT][TX_BUF_SIZE];
static uint8_t tx_buf_used[TX_BUF_COUNT] = {0};

void nexlink_tx_init(void)
{
    txq = xQueueCreate(TX_BUF_COUNT, sizeof(tx_item_t));
}

void nexlink_tx_send(const void *buf, uint16_t len)
{
    if (len > TX_BUF_SIZE)
        return;

    tx_item_t item;
    item.len = len;
    item.buf = (uint8_t *)buf;

    xQueueSend(txq, &item, portMAX_DELAY);
}

extern USBD_HandleTypeDef hUSB;
void NexLinkTxTask(void *arg)
{
    tx_item_t item;
    for (;;)
    {
        if (USBD_NEX_LINK_TxReady(&hUSB))
            if (xQueueReceive(txq, &item, portMAX_DELAY))
            {
                usb_tx(item.buf, item.len);
                tx_buf_free((uint8_t *)item.buf);
            }
    }
}

uint8_t *tx_buf_alloc(void)
{
    for (int i = 0; i < TX_BUF_COUNT; i++)
    {
        if (!tx_buf_used[i])
        {
            tx_buf_used[i] = 1;
            return tx_buf_pool[i];
        }
    }
    return NULL;
}

void tx_buf_free(uint8_t *buf)
{
    for (int i = 0; i < TX_BUF_COUNT; i++)
    {
        if (tx_buf_pool[i] == buf)
        {
            tx_buf_used[i] = 0;
            return;
        }
    }
}
