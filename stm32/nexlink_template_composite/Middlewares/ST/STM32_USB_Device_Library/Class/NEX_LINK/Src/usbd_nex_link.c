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

#include "usbd_nex_link.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include "tim.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "usart.h"
#include "cmsis_os.h"

#include "nexlink_usb_if.h"

typedef struct
{
	uint8_t ep0_buf[USB_CMD_PACKET_SIZE];

	__IO uint8_t txstate;

} USBD_NEX_LINK_HandleTypeDef __attribute__((aligned(4)));

static uint8_t USBD_NEX_LINK_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_NEX_LINK_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_NEX_LINK_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_NEX_LINK_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_NEX_LINK_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_NEX_LINK_GetCfgDesc(uint16_t *len);
static uint8_t USBD_NEX_LINK_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_NEX_LINK_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length);

/* CAN interface class callbacks structure */
USBD_ClassTypeDef USBD_NEX_LINK = {
	USBD_NEX_LINK_Init,
	USBD_NEX_LINK_DeInit,
	USBD_NEX_LINK_Setup,
	NULL, // EP0_TxSent
	USBD_NEX_LINK_EP0_RxReady,
	USBD_NEX_LINK_DataIn,
	USBD_NEX_LINK_DataOut,
	NULL,
	NULL, // IsoInComplete
	NULL, // IsoOutComplete
	USBD_NEX_LINK_GetCfgDesc,
	USBD_NEX_LINK_GetCfgDesc,
	USBD_NEX_LINK_GetCfgDesc,
	NULL,					 // GetDeviceQualifierDescriptor
	USBD_NEX_LINK_GetStrDesc // GetUsrStrDescriptor
};

/* Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_NEX_LINK_CfgDesc[USB_CONFIG_DESC_SIZ] __ALIGN_END =
	{
		/*---------------------------------------------------------------------------*/
		/* Configuration Descriptor */
		0x09,						 /* bLength */
		USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType */
		USB_CONFIG_DESC_SIZ,		 /* wTotalLength */
		0x00,
		0x01, /* bNumInterfaces */
		0x01, /* bConfigurationValue */
		0x00, /* iConfiguration */
		0x80, /* bmAttributes */
		0x4B, /* MaxPower 150 mA */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* GS_USB Interface Descriptor */
		0x09,					 /* bLength */
		USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
		0x00,					 /* bInterfaceNumber */
		0x00,					 /* bAlternateSetting */
		0x02,					 /* bNumEndpoints */
		0xFF,					 /* bInterfaceClass: Vendor Specific*/
		0xFF,					 /* bInterfaceSubClass: Vendor Specific */
		0xFF,					 /* bInterfaceProtocol: Vendor Specific */
		0x00,					 /* iInterface */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* EP1 descriptor */
		0x07,							  /* bLength */
		USB_DESC_TYPE_ENDPOINT,			  /* bDescriptorType */
		GSUSB_ENDPOINT_IN,				  /* bEndpointAddress */
		0x02,							  /* bmAttributes: bulk */
		LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
		HIBYTE(USB_DATA_MAX_PACKET_SIZE),
		0x00, /* bInterval: */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* EP2 descriptor */
		0x07,							  /* bLength */
		USB_DESC_TYPE_ENDPOINT,			  /* bDescriptorType */
		GSUSB_ENDPOINT_OUT,				  /* bEndpointAddress */
		0x02,							  /* bmAttributes: bulk */
		LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
		HIBYTE(USB_DATA_MAX_PACKET_SIZE),
		0x00, /* bInterval: */
			  /*---------------------------------------------------------------------------*/

};

/* Microsoft OS String Descriptor */
__ALIGN_BEGIN uint8_t USBD_NEX_LINK_WINUSB_STR[] __ALIGN_END =
	{
		0x12,					/* length */
		0x03,					/* descriptor type == string */
		0x4D, 0x00, 0x53, 0x00, /* signature: "MSFT100" */
		0x46, 0x00, 0x54, 0x00,
		0x31, 0x00, 0x30, 0x00,
		0x30, 0x00,
		USBD_NEX_LINK_VENDOR_CODE, /* vendor code */
		0x00					   /* padding */
};

/*  Microsoft Compatible ID Feature Descriptor  */
static __ALIGN_BEGIN uint8_t USBD_MS_COMP_ID_FEATURE_DESC[] __ALIGN_END = {
	0x28, 0x00, 0x00, 0x00, /* length */
	0x00, 0x01,				/* version 1.0 */
	0x04, 0x00,				/* descr index (0x0004) */
	0x01,					/* number of sections */
	0x00, 0x00, 0x00, 0x00, /* reserved */
	0x00, 0x00, 0x00,
	0x00,					/* interface number */
	0x01,					/* reserved */
	0x57, 0x49, 0x4E, 0x55, /* compatible ID ("WINUSB\0\0") */
	0x53, 0x42, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, /* sub-compatible ID */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, /* reserved */
	0x00, 0x00};

/* Microsoft Extended Properties Feature Descriptor */
static __ALIGN_BEGIN uint8_t USBD_MS_EXT_PROP_FEATURE_DESC[] __ALIGN_END = {
	0x92, 0x00, 0x00, 0x00, /* length */
	0x00, 0x01,				/* version 1.0 */
	0x05, 0x00,				/* descr index (0x0005) */
	0x01, 0x00,				/* number of sections */
	0x88, 0x00, 0x00, 0x00, /* property section size */
	0x07, 0x00, 0x00, 0x00, /* property data type 7: Unicode REG_MULTI_SZ */
	0x2a, 0x00,				/* property name length */

	0x44, 0x00, 0x65, 0x00, /* property name "DeviceInterfaceGUIDs" */
	0x76, 0x00, 0x69, 0x00,
	0x63, 0x00, 0x65, 0x00,
	0x49, 0x00, 0x6e, 0x00,
	0x74, 0x00, 0x65, 0x00,
	0x72, 0x00, 0x66, 0x00,
	0x61, 0x00, 0x63, 0x00,
	0x65, 0x00, 0x47, 0x00,
	0x55, 0x00, 0x49, 0x00,
	0x44, 0x00, 0x73, 0x00,
	0x00, 0x00,

	0x50, 0x00, 0x00, 0x00, /* property data length */

	0x7b, 0x00, 0x63, 0x00, /* property name: "{c15b4308-04d3-11e6-b3ea-6057189e6443}\0\0" */
	0x31, 0x00, 0x35, 0x00,
	0x62, 0x00, 0x34, 0x00,
	0x33, 0x00, 0x30, 0x00,
	0x38, 0x00, 0x2d, 0x00,
	0x30, 0x00, 0x34, 0x00,
	0x64, 0x00, 0x33, 0x00,
	0x2d, 0x00, 0x31, 0x00,
	0x31, 0x00, 0x65, 0x00,
	0x36, 0x00, 0x2d, 0x00,
	0x62, 0x00, 0x33, 0x00,
	0x65, 0x00, 0x61, 0x00,
	0x2d, 0x00, 0x36, 0x00,
	0x30, 0x00, 0x35, 0x00,
	0x37, 0x00, 0x31, 0x00,
	0x38, 0x00, 0x39, 0x00,
	0x65, 0x00, 0x36, 0x00,
	0x34, 0x00, 0x34, 0x00,
	0x33, 0x00, 0x7d, 0x00,
	0x00, 0x00, 0x00, 0x00};

uint8_t USB_BUFF[1024];

static uint8_t USBD_NEX_LINK_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	UNUSED(cfgidx);
	uint8_t ret = USBD_FAIL;
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)USBD_malloc(sizeof(USBD_NEX_LINK_HandleTypeDef));

	if (hnex == NULL)
	{
		pdev->pClassDataCmsit[pdev->classId] = NULL;
		return USBD_EMEM;
	}

	USBD_memset(hnex, 0, sizeof(USBD_NEX_LINK_HandleTypeDef));

	pdev->pClassDataCmsit[pdev->classId] = hnex;
	pdev->pClassData = hnex;

	if (pdev->pClassData)
	{
		USBD_LL_OpenEP(pdev, GSUSB_ENDPOINT_IN, USBD_EP_TYPE_BULK, USB_DATA_MAX_PACKET_SIZE);
		USBD_LL_OpenEP(pdev, GSUSB_ENDPOINT_OUT, USBD_EP_TYPE_BULK, USB_DATA_MAX_PACKET_SIZE);
		USBD_NEX_LINK_PrepareReceive(pdev);
		ret = USBD_OK;
	}
	else
	{
		ret = USBD_FAIL;
	}

	return ret;
}

static uint8_t USBD_NEX_LINK_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	UNUSED(cfgidx);

	USBD_LL_CloseEP(pdev, GSUSB_ENDPOINT_IN);
	USBD_LL_CloseEP(pdev, GSUSB_ENDPOINT_OUT);

	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	hnex->txstate = 0;
	return USBD_OK;
}

#define USBD_MANUFACTURER_STRING "Template"
const char NAME_STR[] = USBD_MANUFACTURER_STRING;
const char VERSION_STR[] = "V1.00";

static uint8_t USBD_NEX_LINK_Config_Request(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_Vendor_Request(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	return USBD_NEX_LINK_Config_Request(pdev, req);
}

bool USBD_NEX_LINK_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	uint16_t len = 0;
	uint8_t *pbuf;

	if (req->bRequest == USBD_NEX_LINK_VENDOR_CODE)
	{
		switch (req->wIndex)
		{
		case 0x0004:
			pbuf = USBD_MS_COMP_ID_FEATURE_DESC;
			len = sizeof(USBD_MS_COMP_ID_FEATURE_DESC);
			USBD_CtlSendData(pdev, pbuf, MIN(len, req->wLength));
			return true;

		case 0x0005:
			if (req->wValue == 0)
			{ // only return our GUID for interface #0
				pbuf = USBD_MS_EXT_PROP_FEATURE_DESC;
				len = sizeof(USBD_MS_EXT_PROP_FEATURE_DESC);
				USBD_CtlSendData(pdev, pbuf, MIN(len, req->wLength));
				return true;
			}
			break;
		}
	}

	return false;
}

bool USBD_NEX_LINK_CustomInterfaceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	return USBD_NEX_LINK_CustomDeviceRequest(pdev, req);
}

static uint8_t USBD_NEX_LINK_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{

	static uint8_t ifalt = 0;
	switch (req->bmRequest & USB_REQ_TYPE_MASK)
	{

	case USB_REQ_TYPE_CLASS:
	case USB_REQ_TYPE_VENDOR:
		return USBD_NEX_LINK_Vendor_Request(pdev, req);

	case USB_REQ_TYPE_STANDARD:
		switch (req->bRequest)
		{
		case USB_REQ_GET_INTERFACE:
			USBD_CtlSendData(pdev, &ifalt, 1);
			break;

		case USB_REQ_SET_INTERFACE:
		default:
			break;
		}
		break;

	default:
		break;
	}
	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	hnex->txstate = 0;
	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

	uint8_t retval = USBD_FAIL;

	uint32_t rxlen = USBD_LL_GetRxDataSize(pdev, epnum);
	usb_rx_isr(
		(uint8_t *)USB_BUFF,
		rxlen);

	USBD_NEX_LINK_PrepareReceive(pdev);

	return retval;
}

static uint8_t *USBD_NEX_LINK_GetCfgDesc(uint16_t *len)
{

	*len = sizeof(USBD_NEX_LINK_CfgDesc);
	return USBD_NEX_LINK_CfgDesc;
}

inline uint8_t USBD_NEX_LINK_PrepareReceive(USBD_HandleTypeDef *pdev)
{
	return USBD_LL_PrepareReceive(pdev, GSUSB_ENDPOINT_OUT, (uint8_t *)(USB_BUFF), sizeof USB_BUFF);
}

bool USBD_NEX_LINK_TxReady(USBD_HandleTypeDef *pdev)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	return hnex->txstate == 0;
}

uint8_t USBD_NEX_LINK_Transmit(USBD_HandleTypeDef *pdev, uint8_t *buf, uint16_t len)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	if (hnex->txstate == 0)
	{
		hnex->txstate = 1;
		USBD_LL_Transmit(pdev, GSUSB_ENDPOINT_IN, buf, len);
		return USBD_OK;
	}
	else
	{
		return USBD_BUSY;
	}
}

uint8_t *USBD_NEX_LINK_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
	UNUSED(pdev);

	switch (index)
	{
	// case DFU_INTERFACE_STR_INDEX:
	// USBD_GetString(DFU_INTERFACE_STRING_FS, USBD_StrDesc, length);
	// return USBD_StrDesc;
	case 0xEE:
		*length = sizeof(USBD_NEX_LINK_WINUSB_STR);
		return USBD_NEX_LINK_WINUSB_STR;
	default:
		*length = 0;
		USBD_CtlError(pdev, 0);
		return 0;
	}
}
