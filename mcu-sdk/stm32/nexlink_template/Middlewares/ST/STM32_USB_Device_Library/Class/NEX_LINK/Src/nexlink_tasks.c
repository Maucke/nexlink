#include "nexlink_tasks.h"
#include "nexlink_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "nexlink_rampool.h"
#include <queue.h>
#include "usbd_nex_link.h"

TaskHandle_t nexlink_rx_task_handle;
TaskHandle_t nexlink_tx_task_handle;

static QueueHandle_t txq;
static QueueHandle_t rxq;

static uint8_t rx_buf[NL_MAX_PAYLOAD];
static uint16_t rx_len;

void nexlink_init(void)
{
    txq = xQueueCreate(BUF_COUNT, sizeof(item_t));
    rxq = xQueueCreate(BUF_COUNT, sizeof(item_t));
}

void nexlink_send(const void *buf, uint16_t len)
{
    if (len > BUF_SIZE)
        return;

    item_t item;
    item.len = len;
    item.buf = (uint8_t *)buf;

    xQueueSend(txq, &item, portMAX_DELAY);
}

void nexlink_recv_isr(const void *buf, uint16_t len)
{
    BaseType_t woken = pdFALSE;
    if (len > BUF_SIZE)
        return;

    item_t item;
    item.len = len;
    item.buf = (uint8_t *)buf;

    xQueueSendFromISR(rxq, &item, &woken);
    portYIELD_FROM_ISR(woken);
}

void nexlink_rx_bytes(const uint8_t *data, uint16_t len)
{
    memcpy(rx_buf + rx_len, data, len);
    rx_len += len;

    while (rx_len >= HEAD_LEN)
    {
        if (rx_buf[0] != NL_MAGIC)
        {
            memmove(rx_buf, rx_buf + 1, --rx_len);
            continue;
        }

        uint16_t plen = *(uint16_t *)(rx_buf + 6);
        uint16_t total = HEAD_LEN + plen;
        if (rx_len < total) return;

        nl_packet_t *pkt = (nl_packet_t *)rx_buf;
        if (pkt->type == NL_PKT_CMD)
            nexlink_recv_isr(pkt, total);

        memmove(rx_buf, rx_buf + total, rx_len - total);
        rx_len -= total;
    }
}

extern USBD_HandleTypeDef hUSB;
void NexLinkTxTask(void *arg)
{
    item_t item;
    for (;;)
    {
        if (USBD_NEX_LINK_TxReady(&hUSB))
            if (xQueueReceive(txq, &item, portMAX_DELAY))
            {
                USBD_NEX_LINK_Transmit(&hUSB, item.buf, item.len);
            }
    }
}

void NexLinkRxTask(void *arg)
{
    item_t item;
    for (;;)
    {
        if (xQueueReceive(rxq, &item, portMAX_DELAY))
        {
            if (nexlink_frame_valid(item.buf, item.len))
            {
                nl_packet_t *pkt = (nl_packet_t *)item.buf;
                nexlink_dispatch(pkt);
            }
            buf_free(item.buf);
        }
    }
}

void NexLinkInit(void)
{
    nexlink_init();

    xTaskCreate(
        NexLinkTxTask,
        "usb_tx",
        128, NULL, 6, &nexlink_tx_task_handle);

    xTaskCreate(
        NexLinkRxTask,
        "usb_rx",
        512, NULL, 5, &nexlink_rx_task_handle);
}
