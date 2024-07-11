#ifndef LIBUSBCONTROL_H
#define LIBUSBCONTROL_H

#include "libusb.h"

#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

// 定义设备信息结构体
struct libusb_device_info {
    uint16_t vendor_id;
    uint16_t product_id;
    char manufacturer[256];
    char product[256];
    char serial_number[256];
};

extern "C" __declspec(dllexport) int init();
extern "C" __declspec(dllexport) int transfer(unsigned char* data, int length);
extern "C" __declspec(dllexport) int receive(unsigned char* data, int length);
extern "C" __declspec(dllexport) void close();
extern "C" __declspec(dllexport) int control_get(
    uint8_t bRequest, uint16_t wValue,
    unsigned char* data, uint16_t wLength);
extern "C" __declspec(dllexport) int control_set(
    uint8_t bRequest, uint16_t wValue,
    unsigned char* data, uint16_t wLength);

extern "C" __declspec(dllexport) void get_device_info(int index, struct libusb_device_info* info);
#endif // WINUSBDRIVER_H
