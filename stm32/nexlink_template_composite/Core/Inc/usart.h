/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
#define Uart_Max_Length 2048
extern uint8_t Uart_Recv1_Buf[];
extern uint16_t Uart_Recv1_Length;

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
    PROTOCOL_USB         // USB通信
} CommunicationProtocol;

typedef struct {
		unsigned int timestamp;
		unsigned char len;
		CommunicationProtocol type;
		TxRxMode dir;
		unsigned char reserve2;
		unsigned char data[64-8];
} UartData;
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

