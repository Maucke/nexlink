#pragma once
#include <stdint.h>

#define NL_MAGIC 0xA5

#define HEAD_LEN 8
#define NL_MAX_PAYLOAD 1024

#define NL_VERSION_MAJOR  1
#define NL_VERSION_MINOR  1
#define NL_VERSION_PATCH  0
#define NL_VERSION_BUILD  0

/* CMD */
//16-bit cmd = [ 高 8 位：功能域 ][ 低 8 位：子命令 ]
/* 0x0000 - core */
#define CMD_PING            0x0001
#define CMD_GET_VERSION     0x0002
#define CMD_SYNC_TIME       0x0003
#define CMD_LOOPBACK     0x0004

/* 0x0100 - log */
#define EVT_LOG             0x0100
#define EVT_WARN            0x0101
#define EVT_ERROR           0x0102

/* 0x0400 - frame */
#define EVT_FRAME_BEGIN     0x0400
#define EVT_FRAME_DATA      0x0401
#define EVT_FRAME_END       0x0402

typedef enum {
    NL_PKT_CMD   = 0x01,
    NL_PKT_RESP  = 0x02,
    NL_PKT_EVENT = 0x03
} nl_pkt_type_t;

typedef enum
{
    NL_ERR_OK            = 0x00,
    NL_ERR_UNSUPPORTED   = 0x01,
    NL_ERR_INVALID_PARAM = 0x02,
    NL_ERR_BUSY          = 0x03,
    NL_ERR_NOT_READY     = 0x04,
    NL_ERR_INTERNAL      = 0x7F,
} nl_err_t;

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  type;
    uint16_t cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t  payload[];
} nl_packet_t;

typedef struct __attribute__((packed))
{
    uint16_t err;   
    uint8_t  data[];
} nl_resp_t;

typedef struct __attribute__((packed))
{
    uint8_t  major;
    uint8_t  minor;
    uint16_t patch;
    uint32_t build;
} nl_version_t;
