#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t len;
    uint8_t *buf;
} tx_item_t;

void nexlink_tx_init(void);
bool nexlink_tx_send(const void *buf, uint16_t len);
void nexlink_tx_send_isr(const void *buf, uint16_t len);
void nexlink_tx_auto(const void *buf, uint16_t len);
bool nexlink_tx_receive_nb(tx_item_t *item);
