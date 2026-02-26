#pragma once
#include <stdint.h>

#define NEXLINK_VID 0x1D51
#define NEXLINK_PID_MASK 0x6000

/* NexLink USB Product ID */
typedef enum
{
    NEXLINK_PID_UNKNOWN = 0x6000,

    NEXLINK_PID_BASIC = 0x606F,   // 基础型号
    NEXLINK_PID_RES = 0x6060,   // 可编程电阻
    NEXLINK_PID_ADAPT = 06061,   // Adapt型号

} NexLinkPid_t;

int usb_scan(
    char serials[][64],
    char products[][64],
    int max_count);

int usb_open(
    const char *serial,
    void **out);

void usb_close(void *dev);

int usb_bulk_read(
    void *dev,
    void *buf,
    int len,
    int timeout_ms);

int usb_bulk_write(
    void *dev,
    const void *buf,
    int len);
