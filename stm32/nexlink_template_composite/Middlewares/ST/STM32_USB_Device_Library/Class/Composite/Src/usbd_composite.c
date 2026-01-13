/**
 ******************************************************************************
 * @file    usbd_composite.c
 * @author  MCD Application Team
 * @brief   This file provides the HID core functions.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2015 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 * @verbatim
 *
 *          ===================================================================
 *                                COMPOSITE Class  Description
 *          ===================================================================
 *
 *
 *
 *
 *
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 *
 *
 *  @endverbatim
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_composite.h"
#include "usbd_ctlreq.h"

#include "usbd_nex_link.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
/** @addtogroup STM32_USB_DEVICE_LIBRARY
 * @{
 */

/** @defgroup USBD_COMPOSITE
 * @brief usbd core module
 * @{
 */

/** @defgroup USBD_COMPOSITE_Private_TypesDefinitions
 * @{
 */
/**
 * @}
 */

/** @defgroup USBD_COMPOSITE_Private_Defines
 * @{
 */

/**
 * @}
 */

/** @defgroup USBD_COMPOSITE_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @defgroup USBD_COMPOSITE_Private_FunctionPrototypes
 * @{
 */

static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t *USBD_COMPOSITE_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length);
/**
 * @}
 */

/** @defgroup USBD_COMPOSITE_Private_Variables
 * @{
 */

USBD_ClassTypeDef USBD_COMPOSITE_ClassDriver =
    {
        USBD_COMPOSITE_Init,
        USBD_COMPOSITE_DeInit,
        USBD_COMPOSITE_Setup,
        NULL,
        USBD_COMPOSITE_EP0_RxReady,
        USBD_COMPOSITE_DataIn,
        USBD_COMPOSITE_DataOut,
        NULL,
        NULL,
        NULL,
        USBD_COMPOSITE_GetCfgDesc,
        USBD_COMPOSITE_GetCfgDesc,
        USBD_COMPOSITE_GetCfgDesc,
        NULL,
};

#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif /* __ICCARM__ */
/* USB COMPOSITE device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_CfgDesc[USB_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
    {
        /*---------------------------------------------------------------------------*/
        /* Configuration Descriptor */
        0x09,                                  /* bLength */
        USB_DESC_TYPE_CONFIGURATION,           /* bDescriptorType */
        LOBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ), /* wTotalLength */
        HIBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ),
        USBD_INTERFACE_NUM, /* bNumInterfaces */
        0x01,               /* bConfigurationValue */
        0x00,               /* iConfiguration */
        0x80,               /* bmAttributes */
        0x4B,               /* MaxPower 150 mA */
        /*---------------------------------------------------------------------------*/
			
   /* GS_USB Interface Descriptor */
        0x09,                    /* bLength */
        USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
        USBD_INTERFACE_NEXLINK,  /* bInterfaceNumber */
        0x00,                    /* bAlternateSetting */
        0x02,                    /* bNumEndpoints */
        0xFF,                    /* bInterfaceClass: Vendor Specific*/
        0xFF,                    /* bInterfaceSubClass: Vendor Specific */
        0xFF,                    /* bInterfaceProtocol: Vendor Specific */
        0x00,                    /* iInterface */
        /*---------------------------------------------------------------------------*/

        /*---------------------------------------------------------------------------*/
        /* EP1 descriptor */
        0x07,                             /* bLength */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType */
        GSUSB_ENDPOINT_IN,                /* bEndpointAddress */
        0x02,                             /* bmAttributes: bulk */
        LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
        HIBYTE(USB_DATA_MAX_PACKET_SIZE),
        0x00, /* bInterval: */
        /*---------------------------------------------------------------------------*/

        /*---------------------------------------------------------------------------*/
        /* EP2 descriptor */
        0x07,                             /* bLength */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType */
        GSUSB_ENDPOINT_OUT,               /* bEndpointAddress */
        0x02,                             /* bmAttributes: bulk */
        LOBYTE(USB_DATA_MAX_PACKET_SIZE), /* wMaxPacketSize */
        HIBYTE(USB_DATA_MAX_PACKET_SIZE),
        0x01, /* bInterval: */
              /*---------------------------------------------------------------------------*/

        /* IAD Descriptor */
        USBD_IAD_DESC_SIZE,       /* bLength */
        USBD_IAD_DESCRIPTOR_TYPE, /* bDescriptorType */
        USBD_INTERFACE_CDC_CMD,   /* bFirstInterface */
        0x02,                     /* bInterfaceCount */
        0x02,                     /* bFunctionClass */
        0x02,                     /* bFunctionSubClass */
        0x01,                     /* bFunctionProtocol */
        0x00,                     /* iFunction */

        /*---------------------------------------------------------------------------*/
        /* CDC Interface Descriptor */
        /* Interface descriptor type */
        0x09,                     /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
        USBD_INTERFACE_CDC_CMD, /* bInterfaceNumber: Number of Interface */
        0x00,                   /* bAlternateSetting: Alternate setting */
        0x01,                   /* bNumEndpoints: One endpoint used */
        0x02,                   /* bInterfaceClass: Communication Interface Class */
        0x02,                   /* bInterfaceSubClass: Abstract Control Model */
        0x01,                   /* bInterfaceProtocol: Common AT commands */
        0x00,                   /* iInterface */

        /* Header Functional Descriptor */
        0x05, /* bLength: Endpoint Descriptor size */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x00, /* bDescriptorSubtype: Header Func Desc */
        0x10, /* bcdCDC: spec release number */
        0x01,

        /* Call Management Functional Descriptor */
        0x05, /* bFunctionLength */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x01, /* bDescriptorSubtype: Call Management Func Desc */
        0x00, /* bmCapabilities: D0+D1 */
        0x01, /* bDataInterface */

        /* ACM Functional Descriptor */
        0x04, /* bFunctionLength */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x02, /* bDescriptorSubtype: Abstract Control Management desc */
        0x02, /* bmCapabilities */

        /* Union Functional Descriptor */
        0x05,                   /* bFunctionLength */
        0x24,                   /* bDescriptorType: CS_INTERFACE */
        0x06,                   /* bDescriptorSubtype: Union func desc */
        USBD_INTERFACE_CDC_CMD, /* bMasterInterface: Communication class interface */
        USBD_INTERFACE_CDC,     /* bSlaveInterface0: Data Class Interface */

        /* Endpoint 2 Descriptor */
        0x07,                        /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
        CDC_CMD_EP,                  /* bEndpointAddress */
        0x03,                        /* bmAttributes: Interrupt */
        LOBYTE(CDC_CMD_PACKET_SIZE), /* wMaxPacketSize */
        HIBYTE(CDC_CMD_PACKET_SIZE),
        CDC_FS_BINTERVAL, /* bInterval */
        /*---------------------------------------------------------------------------*/

        /* Data class interface descriptor */
        0x09,                    /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_INTERFACE, /* bDescriptorType: */
        USBD_INTERFACE_CDC,      /* bInterfaceNumber: Number of Interface */
        0x00,                    /* bAlternateSetting: Alternate setting */
        0x02,                    /* bNumEndpoints: Two endpoints used */
        0x0A,                    /* bInterfaceClass: CDC */
        0x00,                    /* bInterfaceSubClass */
        0x00,                    /* bInterfaceProtocol */
        0x00,                    /* iInterface */

        /* Endpoint OUT Descriptor */
        0x07,                                /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,              /* bDescriptorType: Endpoint */
        CDC_OUT_EP,                          /* bEndpointAddress */
        0x02,                                /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize */
        HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
        0x00, /* bInterval */

        /* Endpoint IN Descriptor */
        0x07,                                /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,              /* bDescriptorType: Endpoint */
        CDC_IN_EP,                           /* bEndpointAddress */
        0x02,                                /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize */
        HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
        0x00, /* bInterval */
     

};

#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif /* __ICCARM__ */
/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
    {
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x40,
        0x01,
        0x00,
};

/**
 * @}
 */

/** @defgroup USBD_COMPOSITE_Private_Functions
 * @{
 */
typedef struct {
  void *cdc;
  void *nex;
} USBD_CompositeHandleTypeDef;

static USBD_CompositeHandleTypeDef hComposite;

/**
 * @brief  USBD_COMPOSITE_Init
 *         Initialize the COMPOSITE interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    USBD_StatusTypeDef ret;

    pdev->pClassData = NULL;

    ret = USBD_CDC.Init(pdev, cfgidx);
    hComposite.cdc = pdev->pClassData;

    pdev->pClassData = NULL;

    ret = USBD_NEX_LINK.Init(pdev, cfgidx);
    hComposite.nex = pdev->pClassData;

    return ret;
}


/**
 * @brief  USBD_COMPOSITE_Init
 *         DeInitialize the COMPOSITE layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_StatusTypeDef ret;
  pdev->pClassData =  hComposite.cdc;
  ret = USBD_CDC.DeInit(pdev, cfgidx);
  if (ret != USBD_OK)
    return (uint8_t)ret;
  pdev->pClassData = hComposite.nex;
  ret = USBD_NEX_LINK.DeInit(pdev, cfgidx);
  return (uint8_t)ret;
}

/**
 * @brief  USBD_COMPOSITE_Setup
 *         Handle the COMPOSITE specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 */
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev,
                                    USBD_SetupReqTypedef *req)
{
    pdev->pClassData = hComposite.nex;
    USBD_NEX_LINK.Setup(pdev, req);
    pdev->pClassData = hComposite.cdc;
      return USBD_CDC.Setup(pdev, req);
}

/**
 * @brief  USBD_COMPOSITE_GetCfgDesc
 *         return configuration descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t *USBD_COMPOSITE_GetCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_COMPOSITE_CfgDesc);
  return USBD_COMPOSITE_CfgDesc;
}

/**
 * @brief  USBD_COMPOSITE_GetDeviceQualifierDesc
 *         return Device Qualifier descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
uint8_t *USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_COMPOSITE_DeviceQualifierDesc);
  return USBD_COMPOSITE_DeviceQualifierDesc;
}

/**
 * @brief  USBD_COMPOSITE_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch (epnum)
  {
  case (CDC_IN_EP & 0xF):
    pdev->pClassData =  hComposite.cdc;
    USBD_CDC.DataIn(pdev, epnum);
    break;
  case (GSUSB_ENDPOINT_IN & 0xF):
    pdev->pClassData = hComposite.nex;
    USBD_NEX_LINK.DataIn(pdev, epnum);
    break;
  default:
    break;
  }

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_COMPOSITE_EP0_RxReady
 *         handle EP0 Rx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  pdev->pClassData =  hComposite.cdc;
  USBD_CDC.EP0_RxReady(pdev);
  pdev->pClassData = hComposite.nex;
  USBD_NEX_LINK.EP0_RxReady(pdev);

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_COMPOSITE_EP0_TxReady
 *         handle EP0 TRx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_COMPOSITE_EP0_TxReady(USBD_HandleTypeDef *pdev)
{

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_COMPOSITE_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_COMPOSITE_SOF(USBD_HandleTypeDef *pdev)
{

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_COMPOSITE_IsoINIncomplete
 *         handle data ISO IN Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_COMPOSITE_IsoOutIncomplete
 *         handle data ISO OUT Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return (uint8_t)USBD_OK;
}
/**
 * @brief  USBD_COMPOSITE_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  switch (epnum)
  {
  case CDC_OUT_EP:
    pdev->pClassData =  hComposite.cdc;
    USBD_CDC.DataOut(pdev, epnum);
    break;
  case GSUSB_ENDPOINT_OUT:
    pdev->pClassData = hComposite.nex;
    USBD_NEX_LINK.DataOut(pdev, epnum);
    break;
  default:
    break;
  }
  return (uint8_t)USBD_OK;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
