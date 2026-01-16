#pragma once
#include <stdint.h>

#define TX_BUF_SIZE   1024    // 每个 TX buffer 的大小
#define TX_BUF_COUNT  4       // TX buffer 数量

typedef struct
{
    uint16_t len;
    uint8_t *buf;
} tx_item_t;

void nexlink_tx_init(void);
void nexlink_tx_send(const void *buf, uint16_t len);
void NexLinkTxTask(void *arg);

uint8_t *tx_buf_alloc(void);
void tx_buf_free(uint8_t *buf);
