#pragma once
#include <stdint.h>

#define NEXLINK_VID 0x1D51
#define NEXLINK_PID 0x606F

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
