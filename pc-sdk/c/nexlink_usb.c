#include "nexlink_usb.h"
#include "libusb.h"
#include <string.h>

static libusb_context *g_ctx;

int usb_scan(
    char serials[][64],
    int max_count)
{
    libusb_device **list;
    ssize_t cnt;
    int found = 0;

    libusb_init(&g_ctx);
    cnt = libusb_get_device_list(g_ctx, &list);

    for (ssize_t i = 0;
         i < cnt && found < max_count;
         i++)
    {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(list[i], &desc);

        if (desc.idVendor  == NEXLINK_VID &&
            desc.idProduct == NEXLINK_PID)
        {
            libusb_device_handle *h;
            if (libusb_open(list[i], &h) == 0)
            {
                libusb_get_string_descriptor_ascii(
                    h, desc.iSerialNumber,
                    (unsigned char *)serials[found],
                    64);
                libusb_close(h);
                found++;
            }
        }
    }

    libusb_free_device_list(list, 1);
    return found;
}

int usb_open(
    const char *serial,
    void **out)
{
    libusb_device **list;
    ssize_t cnt;

    libusb_init(&g_ctx);
    cnt = libusb_get_device_list(g_ctx, &list);

    for (ssize_t i = 0; i < cnt; i++)
    {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(list[i], &desc);

        if (desc.idVendor  == NEXLINK_VID &&
            desc.idProduct == NEXLINK_PID)
        {
            libusb_device_handle *h;
            if (libusb_open(list[i], &h) == 0)
            {
                char s[64] = {0};
                libusb_get_string_descriptor_ascii(
                    h, desc.iSerialNumber,
                    (unsigned char *)s, sizeof(s));

                if (!serial || strcmp(serial, s) == 0)
                {
                    libusb_claim_interface(h, 0);
                    *out = h;
                    libusb_free_device_list(list, 1);
                    return 0;
                }
                libusb_close(h);
            }
        }
    }

    libusb_free_device_list(list, 1);
    return -1;
}

void usb_close(void *h)
{
    libusb_device_handle *dev = h;
    libusb_release_interface(dev, 0);
    libusb_close(dev);
}

int usb_bulk_read(
    void *h,
    void *buf,
    int len,
    int timeout_ms)
{
    int transferred;
    int rc = libusb_bulk_transfer(
        h, 0x81, buf, len,
        &transferred, timeout_ms);
    return rc == 0 ? transferred : -1;
}

int usb_bulk_write(
    void *h,
    const void *buf,
    int len)
{
    int transferred;
    int rc = libusb_bulk_transfer(
        h, 0x01,
        (unsigned char *)buf,
        len,
        &transferred,
        1000);
    return rc == 0 ? transferred : -1;
}
