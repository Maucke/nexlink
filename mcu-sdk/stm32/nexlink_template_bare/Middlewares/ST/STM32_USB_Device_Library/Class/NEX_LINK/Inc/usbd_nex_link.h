/*

The MIT License (MIT)

Copyright (c) 2016 Hubert Denkmair

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#pragma once

#include <stdbool.h>
#include <usbd_def.h>
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"
//#include "nex_usb.h"

/* Define these here so they can be referenced in other files */

#define GSUSB_ENDPOINT_IN          0x81
#define GSUSB_ENDPOINT_OUT         0x01

#ifdef FUSB
#define USB_DATA_MAX_PACKET_SIZE   64  /* Endpoint IN & OUT Packet size */
#else
#define USB_DATA_MAX_PACKET_SIZE   512  /* HS bulk max MPS; 1024 is illegal */
#endif
#define USB_CMD_PACKET_SIZE        64  /* Control Endpoint Packet size */
#define USB_CONFIG_DESC_SIZ    (32)
#define NUM_USB_CHANNEL             1
#define USBD_NEX_LINK_VENDOR_CODE  0x20

extern USBD_ClassTypeDef USBD_NEX_LINK;

uint8_t USBD_NEX_LINK_PrepareReceive(USBD_HandleTypeDef *pdev);
bool USBD_NEX_LINK_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
bool USBD_NEX_LINK_CustomInterfaceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
bool USBD_NEX_LINK_TxReady(USBD_HandleTypeDef *pdev);
uint8_t USBD_NEX_LINK_Transmit(USBD_HandleTypeDef *pdev, uint8_t *buf, uint16_t len);
//uint8_t USBD_NEX_LINK_GetProtocolVersion(USBD_HandleTypeDef *pdev);
//uint8_t USBD_NEX_LINK_GetPadPacketsToMaxPacketSize(USBD_HandleTypeDef *pdev);
