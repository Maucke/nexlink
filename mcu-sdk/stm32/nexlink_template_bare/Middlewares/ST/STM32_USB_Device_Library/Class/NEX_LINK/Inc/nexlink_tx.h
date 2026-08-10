#pragma once
#include <stdint.h>
#include <stdbool.h>

bool nexlink_tx_send(const void *buf, uint16_t len);
