#pragma once
#include <stdint.h>

#define TX_MAX_LEN 256
#define TX_QUEUE_LEN 8

typedef struct
{
    uint16_t len;
    uint8_t buf[TX_MAX_LEN];
} tx_item_t;

void nexlink_tx_init(void);
void nexlink_tx_send(const void *buf, uint16_t len);
void usb_tx(const void *buf, uint16_t len);
