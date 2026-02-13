#pragma once
#include <stdint.h>

typedef struct
{
    uint16_t len;
    uint8_t *buf;
} tx_item_t;

void nexlink_tx_init(void);
void nexlink_tx_send(const void *buf, uint16_t len);
void nexlink_tx_auto(const void *buf, uint16_t len);
void NexLinkTxTask(void *arg);
