#include <stdio.h>
#include <stdlib.h>
#include "nexlink.h"

#define USB_VID         0x1D50          //USB的产商ID
#define USB_PID         0x606F          //USB的产品ID

#define EP0ADDR         0x01            //Write端口0地址，通道0
#define EP1ADDR         0x81            //Read 端口1地址，通道1
#define EP2ADDR         0x02            //Write端口2地址，通道2
#define EP3ADDR         0x86            //Read 端口3地址，通道3

libusb_device** devs = NULL;
libusb_device_data device_datas[16];
libusb_context* ctx = NULL;

// 填充设备信息结构体的函数
void fill_device_info(libusb_device_data* device_data) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(device_data->dev, &desc);
    if (r < 0) {
        fprintf(stderr, "Failed to get device descriptor\n");
        return;
    }

    device_data->info.vendor_id = desc.idVendor;
    device_data->info.product_id = desc.idProduct;

    unsigned char string_buffer[256];
    int ret = libusb_get_string_descriptor_ascii(device_data->handle, desc.iManufacturer, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(device_data->info.manufacturer, (const char*)string_buffer, sizeof(device_data->info.manufacturer));
        device_data->info.manufacturer[sizeof(device_data->info.manufacturer) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(device_data->info.manufacturer, "Unknown", sizeof(device_data->info.manufacturer));
    }

    ret = libusb_get_string_descriptor_ascii(device_data->handle, desc.iProduct, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(device_data->info.product, (const char*)string_buffer, sizeof(device_data->info.product));
        device_data->info.product[sizeof(device_data->info.product) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(device_data->info.product, "Unknown", sizeof(device_data->info.product));
    }

    ret = libusb_get_string_descriptor_ascii(device_data->handle, desc.iSerialNumber, string_buffer, sizeof(string_buffer));
    if (ret > 0) {
        strncpy_s(device_data->info.serial_number, (const char*)string_buffer, sizeof(device_data->info.serial_number));
        device_data->info.serial_number[sizeof(device_data->info.serial_number) - 1] = '\0'; // 确保以null结尾
    }
    else {
        strncpy_s(device_data->info.serial_number, "Unknown", sizeof(device_data->info.serial_number));
    }
}

extern "C" __declspec(dllexport) void GetDeviceInfo(int index, libusb_device_info* info)
{
    memcpy(info, &(device_datas[index].info), sizeof(struct libusb_device_info));
}

// 从 USB 设备读取数据
extern "C" __declspec(dllexport) int Receive(int index, unsigned char* data, int length, int *length_actual) {
    if (device_datas[index].handle == NULL)
        return -1;
    return libusb_bulk_transfer(device_datas[index].handle, EP1ADDR, data, length, length_actual, 10);
}

// 向 USB 设备写入数据
extern "C" __declspec(dllexport) int Transfer(int index, unsigned char* data, int length, int* length_actual) {
    if (device_datas[index].handle == NULL)
        return -1;
    return libusb_bulk_transfer(device_datas[index].handle, EP2ADDR, data, length, length_actual, 10);
}

extern "C" __declspec(dllexport) int Scan()
{
    int count = 0;

    int ret = libusb_get_device_list(ctx, &devs);
    if (ret < 0)
    {
        fprintf(stderr, "get usb device list error when open\n");
        // libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stdout, "libusb_get_device_list success\n");
    }
    libusb_device_descriptor desc;
    for (int i = 0; NULL != devs[i] && count < 16; i++)
    {
        //printf("start to get device descriptor");
        ret = libusb_get_device_descriptor(devs[i], &desc);
        if (ret < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            // libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stdout, "Find VID:%04X PID:%04X\n", desc.idVendor, desc.idProduct);
            if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
                device_datas[count].dev = devs[i];
                count++;
            }
        }
    }
    for (int i = 0; i < count; i++)
    {
        int ret = libusb_open(device_datas[i].dev, &device_datas[i].handle);
        if (ret < 0)
        {
            device_datas[i].handle = NULL;
            fprintf(stderr, "fail to open usb device\n");
            // libusb_exit(ctx);
            return -1;
        }
        else {
            fprintf(stderr, "libusb_open success\n");
        }
        fill_device_info(&device_datas[i]);
        if (device_datas[i].handle != NULL)
            libusb_close(device_datas[i].handle);
    }
    return count;
}

extern "C" __declspec(dllexport) int Open(int index)
{
    int ret = libusb_open(device_datas[index].dev, &device_datas[index].handle);
    if (ret < 0)
    {
        device_datas[index].handle = NULL;
        fprintf(stderr, "fail to open usb device\n");
        // libusb_exit(ctx);
        return -1;
    }
    else {
        fprintf(stderr, "libusb_open success\n");
    }

    // 打开指定的USB设备
    if (device_datas[index].handle == NULL) {
        fprintf(stderr, "Failed to open device\n");
        // libusb_exit(ctx);
        return -1;
    }

    ret = libusb_get_active_config_descriptor(device_datas[index].dev, &device_datas[index].cfg);
    if (ret < 0) {
        fprintf(stderr, "Fail to get device config_descriptor\n");
        // libusb_exit(ctx);
        return -1;
    }
    else {
        printf("libusb_get_active_config_descriptor success\n");
    }

    for (int j = 0; j < device_datas[index].cfg->bNumInterfaces; j++) {
        const struct libusb_interface_descriptor* interface_desc = (device_datas[index].cfg->interface + j)->altsetting;
        printf("bInterfaceNumber:%d\n", interface_desc->bInterfaceNumber);
        printf("bNumEndpoints:%d\n", interface_desc->bNumEndpoints);
    }

    libusb_free_config_descriptor(device_datas[index].cfg);

    if (libusb_kernel_driver_active(device_datas[index].handle, 0) == 1)
    {
        ret = libusb_detach_kernel_driver(device_datas[index].handle, 0);
        if (ret < 0) {
            printf("Fail to libusb_detach_kernel_driver\n");
            return -1;
        }
    }

    ret = libusb_claim_interface(device_datas[index].handle, 0);
    if (ret < 0) {
        printf("Fail to libusb_claim_interface\n");
        return -1;
    }

    device_datas[index].m_xfer = libusb_alloc_transfer(0);

    return 1;
}

extern "C" __declspec(dllexport) void Close(int index)
{
    //if (devs != NULL)
    //    libusb_free_device_list(devs, 1);
    //if (cfg != NULL)
    //    libusb_free_config_descriptor(cfg);

    if (device_datas[index].handle != NULL)
        libusb_close(device_datas[index].handle);

    device_datas[index].handle = NULL;
}
extern "C" __declspec(dllexport) int Init()
{
    // 初始化libusb库
    int ret = libusb_init(&ctx);
    if (ret < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(ret));
        return -1;
    }
}
extern "C" __declspec(dllexport) void DeInit()
{
    if (ctx != NULL)
        libusb_exit(ctx);
    ctx = NULL;
}


extern "C" __declspec(dllexport) int ControlSet(int index,
    uint8_t bRequest, uint16_t wValue,
    unsigned char* data, uint16_t wLength)
{
    if (device_datas[index].handle == NULL)
        return -1;
    return libusb_control_transfer(device_datas[index].handle,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE, bRequest, wValue, 0,
        data, wLength, 0);
}

extern "C" __declspec(dllexport) int ControlGet(int index,
    uint8_t bRequest, uint16_t wValue,
    unsigned char* data, uint16_t wLength)
{
    if (device_datas[index].handle == NULL)
        return -1;
    return libusb_control_transfer(device_datas[index].handle,
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE, bRequest, wValue, 0,
        data, wLength, 0);
}