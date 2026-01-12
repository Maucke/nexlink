#include "nexlink_tasks.h"
#include "nexlink_ringbuf.h"
#include "nexlink_app.h"
#include "nexlink_tx.h"
#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t nexlink_rx_task_handle;

void NexLinkRxTask(void *arg)
{
    uint8_t buf[64];

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (ringbuf_available())
        {
            uint16_t n = ringbuf_read(buf, sizeof(buf));
            nexlink_rx_bytes(buf, n);
        }
    }
}

void NexLinkInit(void)
{
    ringbuf_init();
    nexlink_tx_init();

    xTaskCreate(
        NexLinkTxTask,
        "usb_tx",
        512, NULL, 6, NULL);

    xTaskCreate(
        NexLinkRxTask,
        "usb_rx",
        512, NULL, 5, &nexlink_rx_task_handle);
}
