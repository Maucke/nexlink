#pragma once
#include <stdint.h>

#define NL_MAGIC 0xA5

#define HEAD_LEN 8
#define NL_MAX_PAYLOAD 1024

#define NL_VERSION_MAJOR  2
#define NL_VERSION_MINOR  1
#define NL_VERSION_PATCH  0
#define NL_VERSION_BUILD  0

/* CMD */
// 16-bit cmd = [ 高 8 位：功能域 ][ 低 8 位：子命令 ]
/* 0x0000 - core */
#define CMD_PING 0x0001
#define CMD_GET_VERSION 0x0002
#define CMD_SYNC_TIME 0x0003
#define CMD_LOOPBACK 0x0004
#define CMD_GET_DISPLAY_INFO 0x0011

/* 0x0100 - log */
#define EVT_LOG 0x0100
#define EVT_WARN 0x0101
#define EVT_ERROR 0x0102

/* 0x0400 - frame：上位机 → 设备，把一张图送到设备屏幕上 */
#define CMD_FRAME_BEGIN 0x0400
#define CMD_FRAME_DATA 0x0401
#define CMD_FRAME_END 0x0402
#define CMD_FRAME_GET 0x0403

/* 设备 → 上位机：把设备屏幕图像传回去（与上位机 EvtFrameUpload* 值对齐） */
#define EVT_FRAME_UPLOAD_BEGIN 0x0404
#define EVT_FRAME_UPLOAD_DATA 0x0405
#define EVT_FRAME_UPLOAD_END 0x0406

/* 像素格式：值必须和上位机 TargetPixelFormat 完全一致 */
typedef enum
{
    NL_BPP_RGB888 = 0,
    NL_BPP_RGB565 = 1,
    NL_BPP_GRAY8 = 2,
    NL_BPP_MONO1 = 3,
    NL_BPP_DUAL2COLOR = 4,
    NL_BPP_DUAL2COLOR_GRAY8 = 5,
    NL_BPP_RGB332 = 6,
} nl_bpp_t;

#define NL_EVT_MASK_LOG (1 << 0)   // 0x010x
#define NL_EVT_MASK_FRAME (1 << 1) // 0x040x
#define NL_EVT_MASK_ERROR (1 << 2)

typedef enum
{
    NL_PKT_CMD = 0x01,  // request
    NL_PKT_RESP = 0x02, // response
    NL_PKT_EVENT = 0x03 // device → host async
} nl_pkt_type_t;

typedef enum
{
    NL_ERR_OK = 0x00,
    NL_ERR_UNSUPPORTED = 0x01,
    NL_ERR_INVALID_PARAM = 0x02,
    NL_ERR_BUSY = 0x03,
    NL_ERR_NOT_READY = 0x04,
    NL_ERR_INTERNAL = 0x7F,
} nl_err_t;

typedef struct __attribute__((packed))
{
    uint8_t magic;
    uint8_t type;
    uint16_t cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t payload[];
} nl_packet_t;

typedef struct __attribute__((packed))
{
    uint16_t err;
    uint8_t data[];
} nl_resp_t;

typedef struct __attribute__((packed))
{
    uint8_t major;
    uint8_t minor;
    uint16_t patch;
    uint32_t build;
} nl_version_t;

/* 一块屏幕的信息。上位机 CmdGetDisplayInfo 的应答 = display_count(1) + 每屏一份本结构 */
typedef struct __attribute__((packed))
{
    uint16_t width;
    uint16_t height;
    uint8_t bpp;     /* nl_bpp_t */
    uint8_t refresh; /* Hz，仅上报给上位机看 */
} nl_display_t;
