/**
  ******************************************************************************
  * @file    usbd_composite_builder.c
  * @brief   Minimal composite configuration descriptor builder for the
  *          STM32 USB Device Library (CDC + NEX_LINK).
  *
  *          Merges the per-class configuration descriptors into a single
  *          composite configuration descriptor and fills the class endpoint /
  *          interface tables (tclasslist) used by the core to route setup
  *          requests and bulk transfers to the right class.
  ******************************************************************************
  */

#include "usbd_composite_builder.h"

#ifdef USE_USBD_COMPOSITE

#include <string.h>

#define USBD_CMPSIT_CONFIG_DESC_MAX_SIZE  256U

/* Composite configuration descriptor (built at registration time) */
__ALIGN_BEGIN static uint8_t USBD_CMPSIT_CfgDesc[USBD_CMPSIT_CONFIG_DESC_MAX_SIZE] __ALIGN_END;
static uint16_t USBD_CMPSIT_CfgDescLen = 0U;
static uint8_t  USBD_CMPSIT_NbInterfaces = 0U;

/* Standard high-speed device qualifier descriptor */
__ALIGN_BEGIN static uint8_t USBD_CMPSIT_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,    /* bLength */
  USB_DESC_TYPE_DEVICE_QUALIFIER, /* bDescriptorType */
  0x00,                          /* bcdUSB */
  0x02,
  0x00,                          /* bDeviceClass */
  0x00,                          /* bDeviceSubClass */
  0x00,                          /* bDeviceProtocol */
  0x40,                          /* bMaxPacketSize0 */
  0x01,                          /* bNumConfigurations */
  0x00,                          /* bReserved */
};

/**
  * @brief  Add a class to the composite device: merge its configuration
  *         descriptor into the composite one and record its interfaces /
  *         endpoints in the class table used for request routing.
  * @param  pdev: device instance
  * @param  pclass: class handle
  * @param  classtype: class type (not used by the descriptor merger)
  * @param  EpAddr: endpoint address hint (unused)
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_CMPSIT_AddClass(USBD_HandleTypeDef *pdev,
                                        USBD_ClassTypeDef *pclass,
                                        USBD_CompositeClassTypeDef classtype,
                                        uint8_t *EpAddr)
{
  USBD_StatusTypeDef ret = USBD_OK;
  uint16_t len = 0U;
  uint16_t remain = 0U;
  uint8_t *pClassCfgDesc = NULL;
  uint8_t *pSrc;
  uint8_t *pDst;

  UNUSED(EpAddr);

  if (pdev->NumClasses == 0U)
  {
    /* Starting a fresh composite device */
    USBD_CMPSIT_CfgDescLen = 0U;
    USBD_CMPSIT_NbInterfaces = 0U;
  }

  if (pclass == NULL)
  {
    return USBD_FAIL;
  }

  /* Get the class configuration descriptor for the active speed */
#ifdef USE_USB_HS
  if (pclass->GetHSConfigDescriptor != NULL)
  {
    pClassCfgDesc = pclass->GetHSConfigDescriptor(&len);
  }
#else
  if (pclass->GetFSConfigDescriptor != NULL)
  {
    pClassCfgDesc = pclass->GetFSConfigDescriptor(&len);
  }
#endif
  else
  {
    ret = USBD_FAIL;
  }

  if ((ret != USBD_OK) || (pClassCfgDesc == NULL) || (len < USB_CONF_DESC_SIZE))
  {
    return USBD_FAIL;
  }

  /* Write the configuration header once, when the composite is empty */
  if (USBD_CMPSIT_CfgDescLen == 0U)
  {
    if (USBD_CMPSIT_CONFIG_DESC_MAX_SIZE < USB_CONF_DESC_SIZE)
    {
      return USBD_FAIL;
    }

    USBD_CMPSIT_CfgDesc[0] = USB_CONF_DESC_SIZE;      /* bLength */
    USBD_CMPSIT_CfgDesc[1] = USB_DESC_TYPE_CONFIGURATION; /* bDescriptorType */
    USBD_CMPSIT_CfgDesc[2] = 0U;                      /* wTotalLength: patched later */
    USBD_CMPSIT_CfgDesc[3] = 0U;
    USBD_CMPSIT_CfgDesc[4] = 0U;                      /* bNumInterfaces: patched later */
    USBD_CMPSIT_CfgDesc[5] = 1U;                      /* bConfigurationValue */
    USBD_CMPSIT_CfgDesc[6] = 0U;                      /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
    USBD_CMPSIT_CfgDesc[7] = 0xC0U;                   /* bmAttributes: self powered */
#else
    USBD_CMPSIT_CfgDesc[7] = 0x80U;                   /* bmAttributes: bus powered */
#endif /* USBD_SELF_POWERED */
    USBD_CMPSIT_CfgDesc[8] = USBD_MAX_POWER;          /* bMaxPower */
    USBD_CMPSIT_CfgDescLen = USB_CONF_DESC_SIZE;
  }

  /* Walk the class descriptor body, skipping its configuration header */
  pSrc = pClassCfgDesc + USB_CONF_DESC_SIZE;
  remain = len - USB_CONF_DESC_SIZE;
  pDst = USBD_CMPSIT_CfgDesc + USBD_CMPSIT_CfgDescLen;

  while (remain > 0U)
  {
    USBD_DescHeaderTypeDef *hdr = (USBD_DescHeaderTypeDef *)pSrc;
    uint8_t blen = hdr->bLength;
    uint8_t btype = hdr->bDescriptorType;

    if ((blen == 0U) || (blen > remain))
    {
      ret = USBD_FAIL;
      break;
    }

    if (USBD_CMPSIT_CfgDescLen + blen > USBD_CMPSIT_CONFIG_DESC_MAX_SIZE)
    {
      ret = USBD_FAIL;
      break;
    }

    if (btype == USB_DESC_TYPE_IAD)
    {
      /* Interface Association Descriptor: point it at the first interface
         this class is about to claim */
      memcpy(pDst, pSrc, blen);
      pDst[2] = USBD_CMPSIT_NbInterfaces; /* bFirstInterface */
    }
    else if (btype == USB_DESC_TYPE_INTERFACE)
    {
      /* Renumber the interface and record it for request routing */
      if (pdev->tclasslist[pdev->classId].NumIf < USBD_MAX_CLASS_INTERFACES)
      {
        pdev->tclasslist[pdev->classId].Ifs[pdev->tclasslist[pdev->classId].NumIf++] = USBD_CMPSIT_NbInterfaces;
      }
      memcpy(pDst, pSrc, blen);
      pDst[2] = USBD_CMPSIT_NbInterfaces; /* bInterfaceNumber */
      USBD_CMPSIT_NbInterfaces++;
    }
    else if (btype == USB_DESC_TYPE_ENDPOINT)
    {
      /* Record the endpoint for transfer routing */
      if (pdev->tclasslist[pdev->classId].NumEps < USBD_MAX_CLASS_ENDPOINTS)
      {
        USBD_EPTypeDef *ep = &pdev->tclasslist[pdev->classId].Eps[pdev->tclasslist[pdev->classId].NumEps];
        ep->add = pSrc[2];              /* bEndpointAddress (with direction) */
        ep->type = pSrc[3] & 0x03U;     /* bmAttributes & transfer type */
        ep->size = pSrc[4];             /* wMaxPacketSize low byte */
        ep->is_used = 1U;
        pdev->tclasslist[pdev->classId].NumEps++;
      }
      memcpy(pDst, pSrc, blen);
    }
    else
    {
      /* Functional / class-specific descriptors: copy verbatim */
      memcpy(pDst, pSrc, blen);
    }

    pDst += blen;
    USBD_CMPSIT_CfgDescLen += blen;
    pSrc += blen;
    remain -= blen;
  }

  if (ret == USBD_OK)
  {
    pdev->tclasslist[pdev->classId].ClassType = classtype;
    pdev->tclasslist[pdev->classId].Active = 1U;
  }

  return ret;
}

/**
  * @brief  Return the composite configuration descriptor.
  * @param  length: pointer to data length variable
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_CMPSIT_GetCfgDesc(uint16_t *length)
{
  /* Patch the total length and interface count before sending */
  USBD_CMPSIT_CfgDesc[2] = LOBYTE(USBD_CMPSIT_CfgDescLen);
  USBD_CMPSIT_CfgDesc[3] = HIBYTE(USBD_CMPSIT_CfgDescLen);
  USBD_CMPSIT_CfgDesc[4] = USBD_CMPSIT_NbInterfaces;

  *length = USBD_CMPSIT_CfgDescLen;
  return USBD_CMPSIT_CfgDesc;
}

uint8_t *USBD_CMPSIT_GetHSConfigDescriptor(uint16_t *length)
{
  return USBD_CMPSIT_GetCfgDesc(length);
}

uint8_t *USBD_CMPSIT_GetFSConfigDescriptor(uint16_t *length)
{
  return USBD_CMPSIT_GetCfgDesc(length);
}

uint8_t *USBD_CMPSIT_GetOtherSpeedConfigDescriptor(uint16_t *length)
{
  return USBD_CMPSIT_GetCfgDesc(length);
}

uint8_t *USBD_CMPSIT_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_CMPSIT_DeviceQualifierDesc);
  return USBD_CMPSIT_DeviceQualifierDesc;
}

/**
  * @brief  Reset the composite configuration descriptor.
  * @param  pdev: device instance
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_CMPST_ClearConfDesc(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  USBD_CMPSIT_CfgDescLen = 0U;
  USBD_CMPSIT_NbInterfaces = 0U;

  return USBD_OK;
}

/* Composite builder interface */
USBD_CMPSIT_TypeDef USBD_CMPSIT =
{
  USBD_CMPSIT_AddClass,
  USBD_CMPSIT_GetHSConfigDescriptor,
  USBD_CMPSIT_GetFSConfigDescriptor,
  USBD_CMPSIT_GetOtherSpeedConfigDescriptor,
  USBD_CMPSIT_GetDeviceQualifierDescriptor,
};

#endif /* USE_USBD_COMPOSITE */
