#include <stdio.h>
#include <stdlib.h>
#include "libusbcontrol.h"

#define USB_VID         0x1D50          //USB的产商ID
#define USB_PID         0x606F          //USB的产品ID

#define EP0ADDR         0x01            //Write端口0地址，通道0
#define EP1ADDR         0x81            //Read 端口1地址，通道1
#define EP2ADDR         0x02            //Write端口2地址，通道2
#define EP3ADDR         0x86            //Read 端口3地址，通道3

libusb_device_handle* handle = NULL;
libusb_context* ctx = NULL;

// 从 USB 设备读取数据
extern "C" __declspec(dllexport) int receive(unsigned char* data, int length) {
    if (handle == NULL)
        return -1;
    int transferred;
    int ret = libusb_bulk_transfer(handle, EP1ADDR, data, length, &transferred, 10);
    if (ret == 0) {
        //printf("Read %d bytes\n", transferred);
        // 在 data 缓冲区中可以找到接收到的数据
    }
    else {
        if (ret != -7)
            fprintf(stderr, "Error reading from endpoint: %s-%d\n", libusb_error_name(ret), ret);
        transferred = -1;
    }
    return transferred;
}

// 向 USB 设备写入数据
extern "C" __declspec(dllexport) int transfer(unsigned char* data, int length) {
    if (handle == NULL)
        return -1;
    int transferred;
    int ret = libusb_bulk_transfer(handle, EP2ADDR, data, length, &transferred, 10);
    if (ret == 0) {
        //printf("Write %d bytes\n", transferred);
        // 数据成功发送到设备
    }
    else {
        fprintf(stderr, "Error writing to endpoint: %s-%d\n", libusb_error_name(ret), ret);
        transferred = -1;
    }
    return transferred;
}

extern "C" __declspec(dllexport) int scandevices()
{
    libusb_device** devs, * dev;
    struct libusb_device_descriptor desc;
    int num = 0;

    // 初始化libusb库
    int ret = libusb_init(&ctx);
    if (ret < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(ret));
        return -1;
    }

    ret = libusb_get_device_list(ctx, &devs);
    if (ret < 0)
    {
        fprintf(stderr, "get usb device list error when open\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stderr, "libusb_get_device_list success\n");
    }
    int i = 0;
    while ((dev = devs[i++]) != NULL)
    {
        //printf("start to get device descriptor");
        ret = libusb_get_device_descriptor(dev, &desc);
        if (ret < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stderr, "Find VID:%04X PID:%04X\n", desc.idVendor, desc.idProduct);
            if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
                num++;
            }
        }
    }
    libusb_free_device_list(devs, 1);
    return num;
}

extern "C" __declspec(dllexport) int initwithindex(int index)
{
    libusb_device** devs, * dev;
    struct libusb_device_descriptor desc;
    libusb_config_descriptor* cfg = NULL;
    struct libusb_transfer* m_xfer;
    int num = 0;

    int ret = libusb_get_device_list(ctx, &devs);
    if (ret < 0)
    {
        fprintf(stderr, "get usb device list error when open\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stderr, "libusb_get_device_list success\n");
    }
    int i = 0;
    while ((dev = devs[i++]) != NULL)
    {
        //printf("start to get device descriptor");
        ret = libusb_get_device_descriptor(dev, &desc);
        if (ret < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stderr, "Find VID:%04X PID:%04X\n", desc.idVendor, desc.idProduct);
            if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
                if (num == index)
                {
                    ret = libusb_open(dev, &handle);
                    if (ret < 0)
                    {
                        handle = NULL;
                        fprintf(stderr, "fail to open usb device\n");
                        libusb_exit(ctx);
                        return -1;
                    }
                    else {
                        fprintf(stderr, "libusb_open success\n");
                        break;
                    }
                }
                num++;
            }
        }
    }
    libusb_free_device_list(devs, 1);

    // 打开指定的USB设备
    if (handle == NULL) {
        fprintf(stderr, "Failed to open device\n");
        libusb_exit(ctx);
        return -1;
    }

    ret = libusb_get_active_config_descriptor(dev, &cfg);
    if (ret < 0) {
        printf("Fail to get device config_descriptor\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        printf("libusb_get_active_config_descriptor success\n");
    }

    for (int j = 0; j < cfg->bNumInterfaces; j++) {
        const struct libusb_interface_descriptor* interface_desc = (cfg->interface + j)->altsetting;
        printf("bInterfaceNumber:%d\n", interface_desc->bInterfaceNumber);
        printf("bNumEndpoints:%d\n", interface_desc->bNumEndpoints);
    }

    libusb_free_config_descriptor(cfg);

    if (libusb_kernel_driver_active(handle, 0) == 1)
    {
        ret = libusb_detach_kernel_driver(handle, 0);
        if (ret < 0) {
            printf("Fail to libusb_detach_kernel_driver\n");
            return -1;
        }
    }

    ret = libusb_claim_interface(handle, 0);
    if (ret < 0) {
        printf("Fail to libusb_claim_interface\n");
        return -1;
    }

    m_xfer = libusb_alloc_transfer(0);

    return 1;
}

extern "C" __declspec(dllexport) int init()
{
    libusb_device** devs, * dev;
    struct libusb_device_descriptor desc;
    libusb_config_descriptor* cfg = NULL;
    struct libusb_transfer* m_xfer;

    // 初始化libusb库
    int ret = libusb_init(&ctx);
    if (ret < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(ret));
        return -1;
    }

    ret = libusb_get_device_list(ctx, &devs);
    if (ret < 0)
    {
        fprintf(stderr, "get usb device list error when open\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stderr, "libusb_get_device_list success\n");
    }
    int i = 0;
    while ((dev = devs[i++]) != NULL)
    {
        //printf("start to get device descriptor");
        ret = libusb_get_device_descriptor(dev, &desc);
        if (ret < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stderr, "Find VID:%04X PID:%04X\n", desc.idVendor, desc.idProduct);
            if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
                ret = libusb_open(dev, &handle);
                if (ret < 0)
                {
                    handle = NULL;
                    fprintf(stderr, "fail to open usb device\n");
                    libusb_exit(ctx);
                    return -1;
                }
                else {
                    fprintf(stderr, "libusb_open success\n");
                    break;
                }
            }
        }
    }
    libusb_free_device_list(devs, 1);

    // 打开指定的USB设备
    if (handle == NULL) {
        fprintf(stderr, "Failed to open device\n");
        libusb_exit(ctx);
        return -1;
    }

    ret = libusb_get_active_config_descriptor(dev, &cfg);
    if (ret < 0) {
        printf("Fail to get device config_descriptor\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        printf("libusb_get_active_config_descriptor success\n");
    }

    for (int j = 0; j < cfg->bNumInterfaces; j++) {
        const struct libusb_interface_descriptor* interface_desc = (cfg->interface + j)->altsetting;
        printf("bInterfaceNumber:%d\n", interface_desc->bInterfaceNumber);
        printf("bNumEndpoints:%d\n", interface_desc->bNumEndpoints);
    }

    libusb_free_config_descriptor(cfg);

    if (libusb_kernel_driver_active(handle, 0) == 1)
    {
        ret = libusb_detach_kernel_driver(handle, 0);
        if (ret < 0) {
            printf("Fail to libusb_detach_kernel_driver\n");
            return -1;
        }
    }

    ret = libusb_claim_interface(handle, 0);
    if (ret < 0) {
        printf("Fail to libusb_claim_interface\n");
        return -1;
    }

    m_xfer = libusb_alloc_transfer(0);

    return 1;
}

extern "C" __declspec(dllexport) void close()
{
    if (handle != NULL)
        libusb_close(handle);

    if (ctx != NULL)
        libusb_exit(ctx); 
    handle = NULL;
    ctx = NULL;
}

extern "C" __declspec(dllexport) int control_set(
    uint8_t bRequest, uint16_t wValue, 
    unsigned char* data, uint16_t wLength)
{
    if (handle == NULL)
        return -1;
    return libusb_control_transfer(handle,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,  bRequest,  wValue,  0,
        data,  wLength, 0);
}

extern "C" __declspec(dllexport) int control_get(
    uint8_t bRequest, uint16_t wValue,
    unsigned char* data, uint16_t wLength)
{
    if (handle == NULL)
        return -1;
    return libusb_control_transfer(handle,
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE, bRequest, wValue, 0,
        data, wLength, 0);
}