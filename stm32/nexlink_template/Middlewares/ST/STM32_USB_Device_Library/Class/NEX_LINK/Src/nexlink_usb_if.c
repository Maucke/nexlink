#include "nexlink_usb_if.h"
#include "nexlink_ringbuf.h"
#include "usbd_nex_link.h"
#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t nexlink_rx_task_handle;
extern USBD_HandleTypeDef hUSB;

void usb_rx_isr(const uint8_t *buf, uint16_t len)
{
    BaseType_t hpw = pdFALSE;

    ringbuf_write_from_isr(buf, len);

    vTaskNotifyGiveFromISR(
        nexlink_rx_task_handle,
        &hpw);

    portYIELD_FROM_ISR(hpw);
}



void usb_tx(const void *buf, uint16_t len)
{
		USBD_NEX_LINK_Transmit(&hUSB, (uint8_t*)buf, len);
}
