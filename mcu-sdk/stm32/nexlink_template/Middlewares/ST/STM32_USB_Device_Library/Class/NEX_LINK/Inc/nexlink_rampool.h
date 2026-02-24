#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define BUF_SIZE   1024    // 每个 TX buffer 的大小
#define BUF_COUNT  4       // TX buffer 数量

#define SMALL_BUF_SIZE 64
#define SMALL_BUF_COUNT 20

uint8_t *buf_alloc(uint16_t need);
void buf_free(uint8_t *buf);
