#include "nexlink_usb.h"
#include "libusb.h"
#include <string.h>

static libusb_context *g_ctx;

typedef struct
{
    libusb_device_handle* handle;
    int interface_number;
} nexlink_usb_t;

int usb_scan(
    char serials[][64],
    char products[][64],
    int max_count)
{
    libusb_device** list;
    ssize_t cnt;
    int found = 0;

    libusb_init(&g_ctx);
    cnt = libusb_get_device_list(g_ctx, &list);

    for (ssize_t i = 0; i < cnt && found < max_count; i++)
    {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(list[i], &desc);

        if (desc.idVendor == NEXLINK_VID &&
            (desc.idProduct & 0xFF00) == NEXLINK_PID_MASK)
        {
            libusb_device_handle* h;
            if (libusb_open(list[i], &h) == 0)
            {
                libusb_get_string_descriptor_ascii(
                    h,
                    desc.iSerialNumber,
                    (unsigned char*)serials[found],
                    64);

                libusb_get_string_descriptor_ascii(
                    h,
                    desc.iProduct,
                    (unsigned char*)products[found],
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
    const char* serial,
    void** out)
{
    libusb_device** list;
    ssize_t cnt;
    int ret = -1;

    libusb_context* ctx = NULL;
    libusb_device_handle* handle = NULL;

    if (libusb_init(&ctx) != 0)
        return -1;

    cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0)
        goto out;

    for (ssize_t i = 0; i < cnt; i++)
    {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(list[i], &desc) != 0)
            continue;

        if (desc.idVendor != NEXLINK_VID ||
            (desc.idProduct & 0xFF00) != NEXLINK_PID_MASK)
            continue;

        if (libusb_open(list[i], &handle) != 0)
            continue;

        /* ---------- serial match ---------- */
        if (serial)
        {
            char s[64] = { 0 };
            if (desc.iSerialNumber)
            {
                libusb_get_string_descriptor_ascii(
                    handle,
                    desc.iSerialNumber,
                    (unsigned char*)s,
                    sizeof(s));
            }

            if (strcmp(serial, s) != 0)
            {
                libusb_close(handle);
                handle = NULL;
                continue;
            }
        }

        /* ---------- auto find vendor interface ---------- */
        struct libusb_config_descriptor* cfg = NULL;
        if (libusb_get_active_config_descriptor(
            libusb_get_device(handle), &cfg) != 0)
        {
            libusb_close(handle);
            handle = NULL;
            continue;
        }
        nexlink_usb_t* usb = NULL;
        int found_if = -1;

        for (int if_idx = 0; if_idx < cfg->bNumInterfaces; if_idx++)
        {
            const struct libusb_interface* itf =
                &cfg->interface[if_idx];

            for (int alt = 0; alt < itf->num_altsetting; alt++)
            {
                const struct libusb_interface_descriptor* alt_desc =
                    &itf->altsetting[alt];

                if (alt_desc->bInterfaceClass !=
                    LIBUSB_CLASS_VENDOR_SPEC)
                    continue;

                int has_in = 0, has_out = 0;

                for (int ep = 0; ep < alt_desc->bNumEndpoints; ep++)
                {
                    const struct libusb_endpoint_descriptor* epd =
                        &alt_desc->endpoint[ep];

                    if ((epd->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                        != LIBUSB_TRANSFER_TYPE_BULK)
                        continue;

                    if (epd->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                        has_in = 1;
                    else
                        has_out = 1;
                }

                if (has_in && has_out)
                {
                    found_if = alt_desc->bInterfaceNumber;
                    break;
                }
            }

            if (found_if >= 0)
                break;
        }

        libusb_free_config_descriptor(cfg);

        if (found_if < 0)
        {
            libusb_close(handle);
            handle = NULL;
            continue;
        }

        /* ---------- detach kernel driver if needed ---------- */
        if (libusb_kernel_driver_active(handle, found_if) == 1)
            libusb_detach_kernel_driver(handle, found_if);

        if (libusb_claim_interface(handle, found_if) != 0)
        {
            libusb_close(handle);
            handle = NULL;
            continue;
        }

        /* ---------- success ---------- */
        usb = calloc(1, sizeof(nexlink_usb_t));
        usb->handle = handle;
        usb->interface_number = found_if;

        *out = usb;
        ret = 0;
        goto out;
    }

out:
    libusb_free_device_list(list, 1);
    if (ret != 0 && ctx)
        libusb_exit(ctx);
    else
        g_ctx = ctx;

    return ret;
}


void usb_close(void *dev)
{
    if (!dev)
        return;
    nexlink_usb_t* usb = (nexlink_usb_t*)dev;

    if (usb->handle)
    {
        /* release interface */
        libusb_release_interface(
            usb->handle,
            usb->interface_number);

        /* optional: reattach kernel driver */
        libusb_attach_kernel_driver(
            usb->handle,
            usb->interface_number);

        libusb_close(usb->handle);
    }

    free(usb);
}

int usb_bulk_read(
    void *dev,
    void *buf,
    int len,
    int timeout_ms)
{
    nexlink_usb_t* usb = (nexlink_usb_t*)dev;
    int transferred;
    int rc = libusb_bulk_transfer(
        usb->handle, 0x81, buf, len,
        &transferred, timeout_ms);
    return rc == 0 ? transferred : rc;
}

int usb_bulk_write(
    void *dev,
    const void *buf,
    int len)
{
    nexlink_usb_t* usb = (nexlink_usb_t*)dev;
    int transferred;
    int rc = libusb_bulk_transfer(
        usb->handle, 0x01,
        (unsigned char *)buf,
        len,
        &transferred,
        1000);
    return rc == 0 ? transferred : -1;
}
