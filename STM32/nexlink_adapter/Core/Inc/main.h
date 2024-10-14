/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define DEBUG_BUF_SIZE 128


int usb_printf(const char* pcFormat, ...);
#define dbmsg(fmt, args...) printf(""fmt"\r\n", ##args) //__FILE__,
//#define dbmsg(fmt, args...) {} //__FILE__,
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void MX_USB_DEVICE_Init(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_CS_GPIO_Port GPIOA
#define GREEN_LED_Pin GPIO_PIN_14
#define GREEN_LED_GPIO_Port GPIOB
#define RED_LED_Pin GPIO_PIN_15
#define RED_LED_GPIO_Port GPIOB
#define BLUE_LED_Pin GPIO_PIN_8
#define BLUE_LED_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

#define QUEUE_MAX_SIZE 128
#define QUEUE_LOG_SIZE 64

typedef enum{
    Tx = 0,  // 发送
    Rx        // 接收
} TxRxMode;

typedef enum{
    PROTOCOL_LOG,   // 提示信息通信
    PROTOCOL_UART = 1,   // 串口通信
    PROTOCOL_I2C,        // I2C通信
    PROTOCOL_SPI,        // SPI通信
    PROTOCOL_CAN,        // CAN通信
    PROTOCOL_ETHERNET,   // 以太网通信
    PROTOCOL_USB,         // USB通信
    PROTOCOL_ERR = 0xff   // 错误
} CommunicationProtocol;

typedef struct {
		unsigned int timestamp;
		unsigned char len;
		CommunicationProtocol type;
		TxRxMode dir;
		unsigned char iserr;
		unsigned char data[64-8];
} LOGData;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
