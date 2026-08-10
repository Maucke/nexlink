#pragma once
#include <stdint.h>

void usb_rx_isr(const uint8_t *buf, uint16_t len);

void usb_tx(const void *buf, uint16_t len);
