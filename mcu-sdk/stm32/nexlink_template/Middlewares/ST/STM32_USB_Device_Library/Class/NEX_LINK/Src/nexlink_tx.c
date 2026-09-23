#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usbd_def.h"
#include <string.h>
#include "usbd_nex_link.h"
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

void nexlink_tx_reset(void)
{
		xQueueReset(txq);
		buf_free_all();
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

extern USBD_HandleTypeDef hUSB;

void NexLinkTxTask(void *arg)
{
    tx_item_t item;
    const uint32_t SPIN_LIMIT = 1000;   /* 自旋次数，可按 CPU 频率/延迟需求调整 */

    for (;;)
    {
        uint32_t spin = 0;

        /* 混合自旋：先忙等 TxReady，超时后再让出 CPU */
        while (!USBD_NEX_LINK_TxReady(&hUSB))
        {
            if (++spin >= SPIN_LIMIT)
            {
                osDelay(1);             /* 让出 CPU，避免空转 */
                spin = 0;
            }
        }

        /* 此时 USB 已就绪，非阻塞取队列项 */
        if (xQueueReceive(txq, &item, 0) == pdTRUE)
        {
            usb_tx(item.buf, item.len);
        }
        else
        {
            /* 队列为空：短暂让出，避免纯自旋占满 CPU */
            osDelay(1);
        }
    }
}
