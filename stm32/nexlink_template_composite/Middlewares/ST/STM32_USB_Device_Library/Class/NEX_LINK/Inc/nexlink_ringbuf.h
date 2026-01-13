#pragma once
#include <stdint.h>
#include <stdbool.h>

void ringbuf_init(void);
bool ringbuf_write_from_isr(const uint8_t *data, uint16_t len);
uint16_t ringbuf_read(uint8_t *out, uint16_t maxlen);
uint16_t ringbuf_available(void);
