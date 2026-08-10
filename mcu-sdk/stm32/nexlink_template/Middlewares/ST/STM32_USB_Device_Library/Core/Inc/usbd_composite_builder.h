/**
  ******************************************************************************
  * @file    usbd_composite_builder.h
  * @brief   Minimal composite configuration descriptor builder for the
  *          STM32 USB Device Library (CDC + NEX_LINK).
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_COMPOSITE_BUILDER_H
#define __USBD_COMPOSITE_BUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_def.h"

#ifdef USE_USBD_COMPOSITE

typedef struct
{
  USBD_StatusTypeDef (*AddClass)(USBD_HandleTypeDef *pdev,
                                 USBD_ClassTypeDef *pclass,
                                 USBD_CompositeClassTypeDef classtype,
                                 uint8_t *EpAddr);
  uint8_t *(*GetHSConfigDescriptor)(uint16_t *length);
  uint8_t *(*GetFSConfigDescriptor)(uint16_t *length);
  uint8_t *(*GetOtherSpeedConfigDescriptor)(uint16_t *length);
  uint8_t *(*GetDeviceQualifierDescriptor)(uint16_t *length);
} USBD_CMPSIT_TypeDef;

extern USBD_CMPSIT_TypeDef USBD_CMPSIT;

USBD_StatusTypeDef USBD_CMPSIT_AddClass(USBD_HandleTypeDef *pdev,
                                        USBD_ClassTypeDef *pclass,
                                        USBD_CompositeClassTypeDef classtype,
                                        uint8_t *EpAddr);

USBD_StatusTypeDef USBD_CMPST_ClearConfDesc(USBD_HandleTypeDef *pdev);

#endif /* USE_USBD_COMPOSITE */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_COMPOSITE_BUILDER_H */
