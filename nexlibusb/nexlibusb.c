#include "nexlibusb.h"
#include <stdio.h>

UsbDevice** usbdevices = NULL;

__declspec(dllexport) void usb_init() {
    libusb_init(NULL);
}

__declspec(dllexport) void usb_get_info(int index, UsbDevice_Info* info)
{
    UsbDevice* device = usbdevices[index];
    strcpy_s(info->manufacturer, MAX_MANUFACTURER_LENGTH, device->info.manufacturer);
    strcpy_s(info->product, MAX_MANUFACTURER_LENGTH, device->info.product);
    strcpy_s(info->serial_number, MAX_MANUFACTURER_LENGTH, device->info.serial_number);
}

__declspec(dllexport) int usb_find_devices(int vid, int pid) {
    libusb_device** devices;
    ssize_t count = libusb_get_device_list(NULL, &devices);
    int foundCount = 0;
    ssize_t i;
    for (i = 0; i < count; i++) {
        libusb_device* device = devices[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0) {
            fprintf(stderr, "Error getting device descriptor\n");
            continue;
        }
        if (desc.idVendor == vid && desc.idProduct == pid) {

            usbdevices = (UsbDevice**)realloc(usbdevices, (foundCount + 1) * sizeof(UsbDevice*));
            usbdevices[foundCount] = (UsbDevice*)malloc(sizeof(UsbDevice));
            usbdevices[foundCount]->vid = vid;
            usbdevices[foundCount]->pid = pid;
            usbdevices[foundCount]->handle = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
            if (usbdevices[foundCount]->handle != NULL)
            {
                libusb_get_string_descriptor_ascii(usbdevices[foundCount]->handle, desc.iManufacturer, usbdevices[foundCount]->info.manufacturer, MAX_MANUFACTURER_LENGTH);
                libusb_get_string_descriptor_ascii(usbdevices[foundCount]->handle, desc.iProduct, usbdevices[foundCount]->info.product, MAX_MANUFACTURER_LENGTH);
                libusb_get_string_descriptor_ascii(usbdevices[foundCount]->handle, desc.iSerialNumber, usbdevices[foundCount]->info.serial_number, MAX_MANUFACTURER_LENGTH);
                libusb_close(usbdevices[foundCount]->handle);
            }
            foundCount++;
        }
    }
    libusb_free_device_list(devices, 1);
    return foundCount;
}

__declspec(dllexport) int usb_open_device(int index) {

    UsbDevice* usbdevice = usbdevices[index];
    struct libusb_config_descriptor* cfg; 
    usbdevice->handle = libusb_open_device_with_vid_pid(NULL, usbdevice->vid, usbdevice->pid);
    if (usbdevice->handle == NULL) {
        fprintf(stderr, "Fail to open device\n");
        return -1;
    }
    int ret = libusb_get_active_config_descriptor(libusb_get_device(usbdevice->handle), &cfg);
    if (ret < 0) {
        fprintf(stderr, "Fail to get device config_descriptor\n");
        return -2;
    }
    for (int j = 0; j < cfg->bNumInterfaces; j++) {
        const struct libusb_interface_descriptor* interface_desc = (cfg->interface + j)->altsetting;
        fprintf(stderr, "bInterfaceNumber:%d\n", interface_desc->bInterfaceNumber);
        fprintf(stderr, "bNumEndpoints:%d\n", interface_desc->bNumEndpoints);
    }

    libusb_free_config_descriptor(cfg);
    ret = libusb_claim_interface(usbdevice->handle, 0);
    if (ret < 0) {
        fprintf(stdout, "Fail to libusb_claim_interface\n");
        return -3;
    }
    libusb_alloc_transfer(0);
    return ret;
}

__declspec(dllexport) int usb_close_device(int index) {
    UsbDevice* usbdevice = usbdevices[index];

    libusb_release_interface(usbdevice->handle, 0);
    libusb_close(usbdevice->handle);
    return 0;
}

__declspec(dllexport) int usb_write_control(int index, unsigned char requestType, unsigned char request, unsigned short value, unsigned char* data, unsigned int length) {
    UsbDevice* usbdevice = usbdevices[index];

    return libusb_control_transfer(usbdevice->handle, requestType, request, value, 0, data, length, 1000);
}

__declspec(dllexport) int usb_read_control(int index, unsigned char requestType, unsigned char request, unsigned short value, unsigned char* data, unsigned int length) {
    UsbDevice* usbdevice = usbdevices[index];

    return libusb_control_transfer(usbdevice->handle, requestType | LIBUSB_ENDPOINT_IN, request, value, 0, data, length, 1000);
}

__declspec(dllexport) int usb_bulk_transfer(int index, unsigned char endpoint, unsigned char* data, int length, int* transferred) {
    UsbDevice* usbdevice = usbdevices[index];

    return libusb_bulk_transfer(usbdevice->handle, endpoint, data, length, transferred, 1000);
}