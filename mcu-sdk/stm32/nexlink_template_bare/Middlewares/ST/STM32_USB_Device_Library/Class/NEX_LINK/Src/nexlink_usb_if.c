#include "nexlink_usb_if.h"
#include "nexlink_app.h"
#include "usbd_nex_link.h"

extern USBD_HandleTypeDef hUSB;

void usb_rx_isr(const uint8_t *buf, uint16_t len)
{
    nexlink_rx_bytes(buf, len);
}

bool usb_rx_poll(void)
{
    return false;
}

void usb_tx(const void *buf, uint16_t len)
{
    USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)buf, len);
}
