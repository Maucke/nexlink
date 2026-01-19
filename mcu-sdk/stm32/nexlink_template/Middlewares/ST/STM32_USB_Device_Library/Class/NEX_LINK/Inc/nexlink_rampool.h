#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define BUF_SIZE   1024    // 每个 buffer 的大小
#define BUF_COUNT  8       // buffer 数量
uint8_t *buf_alloc(void);
void buf_free(uint8_t *buf);
