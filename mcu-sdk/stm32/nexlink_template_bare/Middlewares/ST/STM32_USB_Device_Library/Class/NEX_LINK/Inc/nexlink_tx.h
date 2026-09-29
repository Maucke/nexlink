#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Queue one frame for the host. Non-blocking and safe to call from the USB
   DataOut ISR; returns false only when the queue is full. */
bool nexlink_tx_send(const void *buf, uint16_t len);

/* Drain one queued frame into the USB stack. Call from the main loop. */
void nexlink_tx_poll(void);
