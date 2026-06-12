#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "usbd_nex_link.h"
#include <string.h>
#include "main.h"

#define TX_BUF_SIZE (1280)

static uint8_t tx_buffer[TX_BUF_SIZE];
static uint16_t tx_len;

extern USBD_HandleTypeDef hUSB;

bool nexlink_tx_send(const void *buf, uint16_t len)
{
    if (len > TX_BUF_SIZE)
        return false;

    /* Wait for previous TX to complete */
    uint32_t timeout = HAL_GetTick() + 3000;
    while (!USBD_NEX_LINK_TxReady(&hUSB))
    {
        if (HAL_GetTick() > timeout)
            return false;
    }

    memcpy(tx_buffer, buf, len);
    tx_len = len;

    usb_tx(tx_buffer, tx_len);
    return true;
}
