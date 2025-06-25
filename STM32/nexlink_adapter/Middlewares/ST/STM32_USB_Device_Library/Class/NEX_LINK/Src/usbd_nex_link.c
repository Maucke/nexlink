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

#include "FreeRTOS.h"
#include "usbd_nex_link.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "stm32f4xx_hal.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"
#include "nex_usb.h"
#include "main.h"
#include "tim.h"
#include "rtc.h"
#include "queue.h"
#include "usart.h"
#include "cmsis_os.h"
#include "i2c.h"

#define USBD_MANUFACTURER_STRING "Adapter"
const char NAME_STR[] = USBD_MANUFACTURER_STRING;
const char VERSION_STR[] = "V1.10";

extern QueueHandle_t xQueue_Log;
extern QueueHandle_t xQueue_Uart;
extern QueueHandle_t xQueue_I2c;

typedef struct
{
	__IO uint32_t txState_Bulk;
	__IO uint32_t txState_Int;
	__IO bool ramindex;
	uint8_t ep0_buf[USB_CMD_PACKET_SIZE];

	USBD_SetupReqTypedef last_setup_request;

	uint8_t *ram_buffer[2];
	uint8_t *rx_buffer;

	long gramdetail;

	nex_usb_des *des;
	nex_i2c_request i2cRequest;
	nex_uart_request uartRequest;

} USBD_NEX_LINK_HandleTypeDef __attribute__((aligned(4)));

static uint8_t USBD_NEX_LINK_Start(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_NEX_LINK_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_NEX_LINK_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_NEX_LINK_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_NEX_LINK_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_NEX_LINK_GetCfgDesc(uint16_t *len);
static uint8_t USBD_NEX_LINK_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_NEX_LINK_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length);
static uint8_t USBD_NEX_LINK_SOF(struct _USBD_HandleTypeDef *pdev);

/* USB interface class callbacks structure */
USBD_ClassTypeDef USBD_NEX_LINK = {
	USBD_NEX_LINK_Start,
	USBD_NEX_LINK_DeInit,
	USBD_NEX_LINK_Setup,
	NULL, // EP0_TxSent
	USBD_NEX_LINK_EP0_RxReady,
	USBD_NEX_LINK_DataIn,
	USBD_NEX_LINK_DataOut,
	USBD_NEX_LINK_SOF,
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
		USB_CONFIG_DESC_SIZ,		 /* wTotalLength - 需要增加7字节(中断端点描述符长度) */
		0x00,
		0x02, /* bNumInterfaces */
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
		0x03,					 /* bNumEndpoints - 从2改为3，增加中断端点 */
		0xFF,					 /* bInterfaceClass: Vendor Specific*/
		0xFF,					 /* bInterfaceSubClass: Vendor Specific */
		0xFF,					 /* bInterfaceProtocol: Vendor Specific */
		0x00,					 /* iInterface */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* EP1 descriptor (Bulk IN) */
		0x07,							  /* bLength */
		USB_DESC_TYPE_ENDPOINT,			  /* bDescriptorType */
		NEXUSB_BULK_ENDPOINT_IN1,		  /* bEndpointAddress */
		0x02,							  /* bmAttributes: bulk */
		LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
		HIBYTE(USB_DATA_MAX_PACKET_SIZE),
		0x00, /* bInterval: */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* EP2 descriptor (Bulk OUT) */
		0x07,							  /* bLength */
		USB_DESC_TYPE_ENDPOINT,			  /* bDescriptorType */
		NEXUSB_BULK_ENDPOINT_OUT1,		  /* bEndpointAddress */
		0x02,							  /* bmAttributes: bulk */
		LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
		HIBYTE(USB_DATA_MAX_PACKET_SIZE),
		0x00, /* bInterval: */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* EP3 descriptor (Interrupt IN) - 新增的中断端点 */
		0x07,						 /* bLength */
		USB_DESC_TYPE_ENDPOINT,		 /* bDescriptorType */
		NEXUSB_INT_ENDPOINT_IN,		 /* bEndpointAddress: EP3 IN */
		0x03,						 /* bmAttributes: Interrupt */
		LOBYTE(USB_CMD_PACKET_SIZE), /* wMaxPacketSize */
		HIBYTE(USB_CMD_PACKET_SIZE),
		0x10, /* bInterval: 1ms for High-Speed, 1ms for Full-Speed */
		/*---------------------------------------------------------------------------*/

		/*---------------------------------------------------------------------------*/
		/* DFU Interface Descriptor */
		/*---------------------------------------------------------------------------*/
		0x09,					 /* bLength */
		USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
		DFU_INTERFACE_NUM,		 /* bInterfaceNumber */
		0x00,					 /* bAlternateSetting */
		0x00,					 /* bNumEndpoints */
		0xFE,					 /* bInterfaceClass: Vendor Specific*/
		0x01,					 /* bInterfaceSubClass */
		0x01,					 /* bInterfaceProtocol : Runtime mode */
		DFU_INTERFACE_STR_INDEX, /* iInterface */

		/*---------------------------------------------------------------------------*/
		/* Run-Time DFU Functional Descriptor */
		/*---------------------------------------------------------------------------*/
		0x09,		/* bLength */
		0x21,		/* bDescriptorType: DFU FUNCTIONAL */
		0x0B,		/* bmAttributes: detach, upload, download */
		0xFF, 0x00, /* wDetachTimeOut */
		0x00, 0x08, /* wTransferSize */
		0x1a, 0x01, /* bcdDFUVersion: 1.1a */
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
	0x40, 0x00, 0x00, 0x00, /* length */
	0x00, 0x01,				/* version 1.0 */
	0x04, 0x00,				/* descr index (0x0004) */
	0x02,					/* number of sections */
	0x00, 0x00, 0x00, 0x00, /* reserved */
	0x00, 0x00, 0x00,
	0x00,					/* interface number */
	0x01,					/* reserved */
	0x57, 0x49, 0x4E, 0x55, /* compatible ID ("WINUSB\0\0") */
	0x53, 0x42, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, /* sub-compatible ID */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, /* reserved */
	0x00, 0x00,
	0x01,					/* interface number */
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

uint8_t USBD_NEX_LINK_Init(USBD_HandleTypeDef *pdev, uint8_t *buffer_a, uint8_t *buffer_b, nex_usb_des *des)
{
	uint8_t ret = USBD_FAIL;
	USBD_NEX_LINK_HandleTypeDef *hnex = calloc(1, sizeof(USBD_NEX_LINK_HandleTypeDef));

	dbmsg("%s", __FUNCTION__);
	if (hnex != 0)
	{
		hnex->ram_buffer[0] = buffer_a;
		hnex->ram_buffer[1] = buffer_b;
		hnex->rx_buffer = hnex->ram_buffer[0];
		hnex->des = des;
		hnex->gramdetail = 0;
		pdev->pClassData = hnex;
		hnex->des->scrdes.width = 0xFFFF;
		hnex->des->scrdes.height = 0xFFFF;
		hnex->des->scrdes.blocksize = 1024;

		ret = USBD_OK;
	}
	else
	{
		pdev->pClassData = 0;
	}

	return ret;
}

static uint8_t USBD_NEX_LINK_Start(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	UNUSED(cfgidx);
	uint8_t ret = USBD_FAIL;
	dbmsg("%s", __FUNCTION__);
	if (pdev->pClassData)
	{
		USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
		USBD_LL_OpenEP(pdev, NEXUSB_BULK_ENDPOINT_IN1, USBD_EP_TYPE_BULK, USB_DATA_MAX_PACKET_SIZE);
		pdev->ep_in[NEXUSB_BULK_ENDPOINT_IN1 & 0xFU].is_used = 1U;

		USBD_LL_OpenEP(pdev, NEXUSB_BULK_ENDPOINT_OUT1, USBD_EP_TYPE_BULK, USB_DATA_MAX_PACKET_SIZE);
		pdev->ep_out[NEXUSB_BULK_ENDPOINT_OUT1 & 0xFU].is_used = 1U;

		pdev->ep_in[NEXUSB_INT_ENDPOINT_IN & 0xFU].bInterval = 0x10;
		USBD_LL_OpenEP(pdev, NEXUSB_INT_ENDPOINT_IN, USBD_EP_TYPE_INTR, USB_CMD_PACKET_SIZE);
		pdev->ep_in[NEXUSB_INT_ENDPOINT_IN & 0xFU].is_used = 1U;
		//		hnex->from_host_buf = queue_pop_front(hnex->q_frame_pool);
		hnex->gramdetail = 0;
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

	dbmsg("%s", __FUNCTION__);
	USBD_LL_CloseEP(pdev, NEXUSB_BULK_ENDPOINT_IN1);
	USBD_LL_CloseEP(pdev, NEXUSB_BULK_ENDPOINT_OUT1);
	USBD_LL_CloseEP(pdev, NEXUSB_INT_ENDPOINT_IN);
	pdev->ep_in[NEXUSB_BULK_ENDPOINT_IN1 & 0xFU].is_used = 0U;
	pdev->ep_out[NEXUSB_BULK_ENDPOINT_OUT1 & 0xFU].is_used = 0U;
	pdev->ep_in[NEXUSB_INT_ENDPOINT_IN & 0xFU].is_used = 0U;
	pdev->ep_in[NEXUSB_INT_ENDPOINT_IN & 0xFU].bInterval = 0;

	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_SOF(struct _USBD_HandleTypeDef *pdev)
{
	//	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef*) pdev->pClassData;
	//	hnex->sof_timestamp_us = timer_get();
	dbmsg("%s", __FUNCTION__);
	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	struct tm *tm_local;
	char time_str[32];
	dbmsg("%s", __FUNCTION__);
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	USBD_SetupReqTypedef *req = &hnex->last_setup_request;

	switch (req->bRequest)
	{

	case NEX_TIMESTAMP_SET:
		memcpy(&hnex->des->timestamp_s, hnex->ep0_buf, sizeof(hnex->des->timestamp_s));
		tm_local = localtime((const time_t *)&hnex->des->timestamp_s); // 转换时间戳
		SYS_SetTime(tm_local);
		SYS_GetTime(tm_local);
		// 格式化时间为字符串
		if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_local) != 0)
		{
			dbmsg("%s", time_str); // 打印时间
		}
		else
		{
			dbmsg("Failed to format time");
		}
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	case NEX_BRIGHTNESS_SET:
		memcpy(&hnex->des->brides, hnex->ep0_buf, sizeof(hnex->des->brides));
		dbmsg("Brightness: %d\n", hnex->des->brides.brightness); // 打印亮度
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	case NEX_SCREEN_SET:
		hnex->gramdetail = 0; // reset pic
		hnex->des->scrdes.direction = ((nex_screen_des *)hnex->ep0_buf)->direction;
		dbmsg("Direction: %d\n", hnex->des->scrdes.direction); // 打印屏幕方向
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	case NEX_I2C_INIT:
		I2C_RateAdjust(&hi2c1, ((nex_i2c_init *)hnex->ep0_buf)->baudRate);
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	case NEX_I2C:
		memcpy(&hnex->i2cRequest, hnex->ep0_buf, sizeof(hnex->i2cRequest));
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	case NEX_UART:
		memcpy(&hnex->uartRequest, hnex->ep0_buf, sizeof(hnex->uartRequest));
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	default:
		USBD_NEX_LINK_PrepareReceive(pdev);
		break;
	}

	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_Config_Request(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;

	dbmsg("%s", __FUNCTION__);
	switch (req->bRequest)
	{

	case NEX_SCREEN_SET:
	case NEX_I2C_INIT:
	case NEX_I2C:
	case NEX_UART_INIT:
	case NEX_UART:
	case NEX_BRIGHTNESS_SET:
	case NEX_TIMESTAMP_SET:
		hnex->last_setup_request = *req;
		USBD_CtlPrepareRx(pdev, hnex->ep0_buf, req->wLength);
		break;
	case NEX_TIMESTAMP_GET:
		//			dbmsg("timestamp_s: %d", sizeof(hnex->des->timestamp_s));
		memcpy(hnex->ep0_buf, &hnex->des->timestamp_s, sizeof(hnex->des->timestamp_s));
		USBD_CtlSendData(pdev, hnex->ep0_buf, sizeof(hnex->des->timestamp_s));
		break;

	case NEX_BRIGHTNESS_GET:
		//			dbmsg("brightness: %d", sizeof(hnex->des->brides.brightness));
		memcpy(hnex->ep0_buf, &hnex->des->brides, sizeof(hnex->des->brides));
		USBD_CtlSendData(pdev, hnex->ep0_buf, sizeof(hnex->des->brides));
		break;

	case NEX_SCREEN_GET:
		//			dbmsg("screen: %d", sizeof(hnex->des->scrdes));
		memcpy(hnex->ep0_buf, &hnex->des->scrdes, sizeof(hnex->des->scrdes));
		USBD_CtlSendData(pdev, hnex->ep0_buf, sizeof(hnex->des->scrdes));
		break;

	case NEX_NAME_GET:
		//			dbmsg("screen: %d", sizeof(hnex->des->scrdes));
		memcpy(hnex->ep0_buf, NAME_STR, sizeof(NAME_STR));
		USBD_CtlSendData(pdev, hnex->ep0_buf, sizeof(NAME_STR));
		break;

	case NEX_VERSION_GET:
		//			dbmsg("screen: %d", sizeof(hnex->des->scrdes));
		memcpy(hnex->ep0_buf, VERSION_STR, sizeof(VERSION_STR));
		USBD_CtlSendData(pdev, hnex->ep0_buf, sizeof(VERSION_STR));
		break;

	case NEX_LOG_GET:
			USBD_CtlError(pdev, req);
		break;
	case NEX_LOG_SIZE_GET:
			USBD_CtlError(pdev, req);
		break;
	default:
		USBD_CtlError(pdev, req);
	}

	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_Vendor_Request(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	dbmsg("%s", __FUNCTION__);
	return USBD_NEX_LINK_Config_Request(pdev, req);
}

bool USBD_NEX_LINK_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	uint16_t len = 0;
	uint8_t *pbuf;
	dbmsg("%s", __FUNCTION__);

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
	dbmsg("%s", __FUNCTION__);
	return USBD_NEX_LINK_CustomDeviceRequest(pdev, req);
}

static uint8_t USBD_NEX_LINK_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	dbmsg("%s", __FUNCTION__);
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
	dbmsg("%s", __FUNCTION__);
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;

	dbmsg("epnum:%d", epnum);
	if (epnum == (NEXUSB_BULK_ENDPOINT_IN1 & 0xF))
		hnex->txState_Bulk = false;
	else if (epnum == (NEXUSB_INT_ENDPOINT_IN & 0xF))
		hnex->txState_Int = false;
	return USBD_OK;
}

static uint8_t USBD_NEX_LINK_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

	UART_HandleTypeDef *uart = &huart1;
	I2C_HandleTypeDef *i2c = &hi2c1;
	dbmsg("%s", __FUNCTION__);
	uint8_t retval = USBD_FAIL;

	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	int rxlen = USBD_LL_GetRxDataSize(pdev, epnum);
	dbmsg("epnum:%d, rxlen: %d", epnum, rxlen);
	USBD_SetupReqTypedef *req = &hnex->last_setup_request;

	if (rxlen > 0)
	{
		switch (req->bRequest)
		{
		case NEX_UART:
			if (hnex->uartRequest.channel == 0)
				uart = &huart1;
			if (HAL_UART_Transmit_DMA(uart, hnex->rx_buffer, rxlen) == HAL_OK)
				retval = USBD_OK;
			break;
		case NEX_I2C:
			if (hnex->i2cRequest.channel == 0)
				i2c = &hi2c1;
			if (hnex->i2cRequest.deviceAddress & 1)
			{
				if (HAL_I2C_Master_Receive_DMA(i2c, hnex->i2cRequest.deviceAddress >> 1, hnex->rx_buffer, rxlen) == HAL_OK)
					retval = USBD_OK;
			}
			else
			{
				if (HAL_I2C_Master_Transmit_DMA(i2c, hnex->i2cRequest.deviceAddress >> 1, hnex->rx_buffer, rxlen) == HAL_OK)
					retval = USBD_OK;
			}
			break;
		}
	}
	else
		retval = USBD_OK;
	return retval;
}

static uint8_t *USBD_NEX_LINK_GetCfgDesc(uint16_t *len)
{
	dbmsg("%s", __FUNCTION__);
	*len = sizeof(USBD_NEX_LINK_CfgDesc);
	return USBD_NEX_LINK_CfgDesc;
}

inline uint8_t USBD_NEX_LINK_PrepareReceive(USBD_HandleTypeDef *pdev)
{
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	hnex->ramindex = (hnex->ramindex + 1) % 2;
	hnex->rx_buffer = hnex->ram_buffer[hnex->ramindex];
	USBD_LL_PrepareReceive(pdev, NEXUSB_BULK_ENDPOINT_OUT1, hnex->rx_buffer, USB_DATA_MAX_PACKET_SIZE);
	return 0;
}

uint8_t USBD_NEX_LINK_Transmit(USBD_HandleTypeDef *pdev, uint8_t *buf, uint16_t len)
{
	dbmsg("%s", __FUNCTION__);
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	if (!hnex->txState_Bulk)
	{
		hnex->txState_Bulk = true;
		USBD_LL_Transmit(pdev, NEXUSB_BULK_ENDPOINT_IN1, buf, len);
		return USBD_OK;
	}
	else
		return USBD_BUSY;
}

uint8_t USBD_NEX_LINK_INT_Transmit(USBD_HandleTypeDef *pdev, uint8_t *buf, uint16_t len)
{
	dbmsg("%s", __FUNCTION__);
	USBD_NEX_LINK_HandleTypeDef *hnex = (USBD_NEX_LINK_HandleTypeDef *)pdev->pClassData;
	if (!hnex->txState_Int)
	{
		hnex->txState_Int = true;
		dbmsg("%s", "USBD_LL_Transmit");
		USBD_LL_Transmit(pdev, NEXUSB_INT_ENDPOINT_IN, buf, len);
		return USBD_OK;
	}
	else
		return USBD_BUSY;
}

uint8_t *USBD_NEX_LINK_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
	UNUSED(pdev);

	dbmsg("%s", __FUNCTION__);
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
