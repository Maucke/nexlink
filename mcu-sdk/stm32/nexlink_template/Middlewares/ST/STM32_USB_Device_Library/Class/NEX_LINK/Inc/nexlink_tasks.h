#pragma once

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t len;
    uint8_t *buf;
} item_t;

void nexlink_init(void);
void nexlink_send(const void *buf, uint16_t len);
void nexlink_recv_isr(const void *buf, uint16_t len);

void nexlink_rx_bytes(const uint8_t *data, uint16_t len);
void NexLinkTxTask(void *arg);
void NexLinkRxTask(void *arg);
	
void NexLinkInit(void);

#ifdef __cplusplus
}
#endif
