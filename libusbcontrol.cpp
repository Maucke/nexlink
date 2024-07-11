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
libusb_config_descriptor* cfg = NULL;
libusb_device** devs = NULL;
libusb_device* userdevs[16];
struct libusb_device_descriptor desc;
libusb_device_info deviceinfos[16];

// 填充设备信息结构体的函数
void fill_device_info(libusb_device* dev, struct libusb_device_info* info) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
        fprintf(stderr, "Failed to get device descriptor\n");
        return;
    }

    info->vendor_id = desc.idVendor;
    info->product_id = desc.idProduct;

    unsigned char string_buffer[256];
    int ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(info->manufacturer, (const char*)string_buffer, sizeof(info->manufacturer));
        info->manufacturer[sizeof(info->manufacturer) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(info->manufacturer, "Unknown", sizeof(info->manufacturer));
    }

    ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(info->product, (const char*)string_buffer, sizeof(info->product));
        info->product[sizeof(info->product) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(info->product, "Unknown", sizeof(info->product));
    }

    ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(info->serial_number, (const char*)string_buffer, sizeof(info->serial_number));
        info->serial_number[sizeof(info->serial_number) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(info->serial_number, "Unknown", sizeof(info->serial_number));
    }
}

extern "C" __declspec(dllexport) void get_device_info(int index, struct libusb_device_info* info)
{
    memcpy(info, &deviceinfos[index], sizeof(struct libusb_device_info));
}
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
    int count = 0;

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
        fprintf(stdout, "libusb_get_device_list success\n");
    }
    for (int i = 0; NULL != devs[i]; i++)
    {
        //printf("start to get device descriptor");
        ret = libusb_get_device_descriptor(devs[i], &desc);
        if (ret < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stdout, "Find VID:%04X PID:%04X\n", desc.idVendor, desc.idProduct);
            if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
                userdevs[count] = devs[i];
                count++;
            }
        }
    }
    for (int i = 0; i < count; i++)
    {
        int ret = libusb_open(userdevs[i], &handle);
        if (ret < 0)
        {
            handle = NULL;
            fprintf(stderr, "fail to open usb device\n");
            libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stderr, "libusb_open success\n");
        }
        fill_device_info(userdevs[i], &deviceinfos[i]);
    }
    return count;
}

extern "C" __declspec(dllexport) int initwithindex(int index)
{
    struct libusb_transfer* m_xfer;
    int ret = libusb_open(userdevs[index], &handle);
    if (ret < 0)
    {
        handle = NULL;
        fprintf(stderr, "fail to open usb device\n");
        libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stderr, "libusb_open success\n");
    }

    // 打开指定的USB设备
    if (handle == NULL) {
        fprintf(stderr, "Failed to open device\n");
        libusb_exit(ctx);
        return -1;
    }

    ret = libusb_get_active_config_descriptor(userdevs[index], &cfg);
    if (ret < 0) {
        fprintf(stderr, "Fail to get device config_descriptor\n");
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
    int ret = scandevices();
    if (ret <= 0) return ret;

    return initwithindex(0);
}

extern "C" __declspec(dllexport) void close()
{
    //if (devs != NULL)
    //    libusb_free_device_list(devs, 1);
    //if (cfg != NULL)
    //    libusb_free_config_descriptor(cfg);

    if (handle != NULL)
        libusb_close(handle);

    if (ctx != NULL)
        libusb_exit(ctx);

    devs = NULL;
    cfg = NULL;
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
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE, bRequest, wValue, 0,
        data, wLength, 0);
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