#pragma once
#include <stdint.h>

#define NL_MAGIC 0xA5

#define HEAD_LEN 8

typedef enum {
    NL_PKT_CMD   = 0x01,
    NL_PKT_RESP  = 0x02,
    NL_PKT_EVENT = 0x03
} nl_pkt_type_t;

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  type;
    uint16_t cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t  payload[];
} nl_packet_t;

/* CMD */
#define CMD_PING       0x01
#define CMD_TIME_SYNC  0x10
