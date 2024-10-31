#ifndef NEXLINKLIB_H
#define NEXLINKLIB_H

#include "libusb.h"

#define MAX_MANUFACTURER_LENGTH 256
typedef struct {
    char manufacturer[MAX_MANUFACTURER_LENGTH];
    char product[MAX_MANUFACTURER_LENGTH];
    char serial_number[MAX_MANUFACTURER_LENGTH];
} UsbDevice_Info;

typedef struct {
    uint16_t vid;
    uint16_t pid;
    UsbDevice_Info info;
    libusb_device* device;
    libusb_device_handle* handle;
} UsbDevice;


__declspec(dllexport) void usb_init();
__declspec(dllexport) void usb_get_info(int index, UsbDevice_Info* info);
__declspec(dllexport) int usb_find_devices(int vid, int pid);
__declspec(dllexport) int usb_open_device(int index);
__declspec(dllexport) int usb_close_device(int index);
__declspec(dllexport) int usb_control_transfer(int index, unsigned char requestType, unsigned char request, unsigned short value, unsigned char* data, unsigned int length, int timeout);
__declspec(dllexport) int usb_bulk_transfer(int index, unsigned char endpoint, unsigned char* data, int length, int* transferred, int timeout);

#endif