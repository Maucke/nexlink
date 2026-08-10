#include "nexlink_tx.h"
#include "nexlink_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "esp_log.h"

#define TAG "nexlink_tx"

QueueHandle_t txq;

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

    xQueueSend(txq, &item, 0);
}

void usb_tx(const void *buf, uint16_t len)
{
    ESP_LOGI(TAG, "usb_tx: len=%d", len);
    uint32_t sent = 0;

    while (sent < len)
    {
        uint32_t avail = tud_vendor_write_available();
        if (avail == 0)
        {
            tud_task();
            vTaskDelay(1);
            continue;
        }

        uint32_t n = tud_vendor_write(buf + sent,
                                      len - sent);
        sent += n;
    }
    tud_vendor_flush();
    tx_buf_free((uint8_t *)buf);
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