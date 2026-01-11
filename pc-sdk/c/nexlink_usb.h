#pragma once
#include <stdint.h>

#define NEXLINK_VID 0x1234
#define NEXLINK_PID 0x5678

int usb_scan(
    char serials[][64],
    int max_count);

int usb_open(
    const char *serial,
    void **out);

void usb_close(void *h);

int usb_bulk_read(
    void *h,
    void *buf,
    int len,
    int timeout_ms);

int usb_bulk_write(
    void *h,
    const void *buf,
    int len);
