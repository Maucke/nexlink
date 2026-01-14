/**
 * This file is part of the NORDIX robotic platform firmware.
 *
 * Copyright (C) 2025 Valerii MAKAROV <v@nordix.dev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tusb.h"

#include <string.h>

#define USB_PID 0x606F
#define USB_VID 0x1D51

#define EPNUM_CDC_NOTIF 0x82
#define EPNUM_CDC_OUT 0x03
#define EPNUM_CDC_IN 0x83
#define EPNUM_VENDOR_OUT 0x01
#define EPNUM_VENDOR_IN 0x81

#define CDC_EP_SIZE 64
#define VENDOR_EP_SIZE 64

#define MS_OS_20_VENDOR_CODE 0x21
#define MS_OS_20_DESC_LEN 0x00B2

#ifndef TUD_BOS_MS_OS_20_DESC_LEN
#define TUD_BOS_MS_OS_20_DESC_LEN 0x1C
#endif

#ifndef MS_OS_20_FEATURE_COMPATIBLE_ID
#ifdef MS_OS_20_FEATURE_COMPATBLE_ID
#define MS_OS_20_FEATURE_COMPATIBLE_ID MS_OS_20_FEATURE_COMPATBLE_ID
#else
#define MS_OS_20_FEATURE_COMPATIBLE_ID 0x03
#endif
#endif

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))

enum
{
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_VENDOR = 2,
  ITF_NUM_TOTAL
};

// ====================== Device Descriptor ======================
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210, // BOS/MS OS 2.0 via BOS
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0211,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01};

const char *string_desc[] = {
    (const char[]){0x09, 0x04}, // 0: en-US (0x0409)
    "Template",                 // 1: Manufacturer
    "NexLinkV2-ESP32S3",        // 2: Product
    "123456789012",             // 3: Serial
    "CDC Debug",                // 4: CDC Interface String (optional)
    "NexLinkV2-ESP32S3"         // 5: USB Interface String
};

// ====================== Configuration Descriptor ======================
enum
{
  CONFIG_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_VENDOR_DESC_LEN
};

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0xA0 /*RWU*/, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4,
                       EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, CDC_EP_SIZE),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 5,
                          EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, VENDOR_EP_SIZE),
};

// ====================== BOS + Microsoft OS 2.0 ======================
#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MS_OS_20_DESC_LEN)

uint8_t const desc_bos[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, MS_OS_20_VENDOR_CODE)};

uint8_t const *tud_descriptor_bos_cb(void)
{
  return desc_bos;
}

static const uint8_t ms_os_20_descriptor_set[] =
    {
        // Set header: length, type, windows version, total length
        U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

        // Configuration subset header: length, type, configuration index, reserved, configuration total length
        U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

        // Function Subset header: length, type, first interface, reserved, subset length
        U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_VENDOR, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),

        // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
        U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

        // MS OS 2.0 Registry property descriptor: length, type
        U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
        U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
        'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
        'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
        U16_TO_U8S_LE(0x0050), // wPropertyDataLength
                               // bPropertyData: “{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}”.
        '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00, '4', 0x00, 'D', 0x00, '9', 0x00, '-', 0x00,
        '0', 0x00, 'D', 0x00, '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00,
        '8', 0x00, 'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00,
        '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00};

static inline void write_array(uint8_t *dst, uint16_t *offset, const void *src, uint16_t len)
{
  memcpy(dst + *offset, src, len);
  *offset += len;
}

bool tud_vendor_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    tusb_control_request_t const *request)
{
  if (stage != CONTROL_STAGE_SETUP)
    return true;

  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
      request->bRequest == MS_OS_20_VENDOR_CODE &&
      request->wIndex == 0x0007)
  {
    uint16_t len = request->wLength;
    if (len > MS_OS_20_DESC_LEN)
      len = MS_OS_20_DESC_LEN;
    return tud_control_xfer(rhport, request,
                            (void *)ms_os_20_descriptor_set, len);
  }
  return false;
}