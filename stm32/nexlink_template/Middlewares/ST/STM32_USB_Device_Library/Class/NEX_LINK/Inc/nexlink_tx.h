#pragma once
#include <stdint.h>

void nexlink_tx_init(void);
void nexlink_tx_send(const void *data, uint16_t len);
void NexLinkTxTask(void *arg);
