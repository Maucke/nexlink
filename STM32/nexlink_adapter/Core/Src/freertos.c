/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "string.h"
#include "rtc.h"
#include "stdio.h"
#include "stdbool.h"
#include "usbd_nex_link.h"
#include "i2c.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
QueueHandle_t xQueue_Uart;
QueueHandle_t xQueue_UartData;
QueueHandle_t xQueue_I2c;
QueueHandle_t xQueue_Log;
osThreadId responseTaskHandle;
osThreadId I2cTaskHandle;
osThreadId UartTaskHandle;
SemaphoreHandle_t xSemaphore_USBDataOut;
//SemaphoreHandle_t xSemaphore_USBDataIn;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartResponseTask(void const * argument);
void StartI2cTask(void const * argument);
void StartUartTask(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
int datalen;
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	xQueue_Log = xQueueCreate( 10,  sizeof(LOGData));
	if( xQueue_Log == NULL )
	{
			/* Queue was not created and must not be used. */
	}
	
	xQueue_UartData = xQueueCreate( QUEUE_MAX_SIZE,  sizeof(LOGData));
	if( xQueue_UartData == NULL )
	{
			/* Queue was not created and must not be used. */
	}
	
	xQueue_Uart = xQueueCreate( 1,  sizeof(nex_uart_request));
	if( xQueue_Uart == NULL )
	{
			/* Queue was not created and must not be used. */
	}
	
	xQueue_I2c = xQueueCreate( 1,  sizeof(nex_i2c_request));
	if( xQueue_I2c == NULL )
	{
			/* Queue was not created and must not be used. */
	}
//	xSemaphore_USBDataIn = xSemaphoreCreateBinary();
//	if( xSemaphore_USBDataIn == NULL )
//	{
//	}
	xSemaphore_USBDataOut = xSemaphoreCreateBinary();
	if( xSemaphore_USBDataOut == NULL )
	{
	}
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadDef(responseTask, StartResponseTask, osPriorityNormal, 0, 128);
  responseTaskHandle = osThreadCreate(osThread(responseTask), NULL);
	
  osThreadDef(I2cTask, StartI2cTask, osPriorityNormal, 0, 128);
  I2cTaskHandle = osThreadCreate(osThread(I2cTask), NULL);
	
  osThreadDef(UartTask, StartUartTask, osPriorityNormal, 0, 128);
  UartTaskHandle = osThreadCreate(osThread(UartTask), NULL);
  /* USER CODE END RTOS_THREADS */

}
extern USBD_HandleTypeDef hUSB;
//__IO 
//
//// DMA 完成回调
//void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
//	USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
//}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
//	struct tm tm_local;
//	char time_str[32];
//	uint8_t readbuff[8];
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
	usb_printf("Hello USB");
//	xSemaphoreGive(xSemaphore_USBDataIn);
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	for(;;)
  { 
//		HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(&hi2c1, 0x64, 0, I2C_MEMADD_SIZE_8BIT, readbuff, 8);
//		if (status != HAL_OK) {
//			uint32_t errorCode = HAL_I2C_GetError(&hi2c1);
//			printf("errorCode:%d \n",errorCode);
//		}
//		printf("Ret:%d, I2c:%02X %02X %02X %02X %02X %02X %02X %02X \n",status, readbuff[0], readbuff[1], readbuff[2], readbuff[3], readbuff[4], readbuff[5], readbuff[6], readbuff[7]);

//		SYS_GetTime(&tm_local);
//		// 格式化时间为字符串
//		if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_local) != 0) {
//				printf("%s\n", time_str); // 打印时间
//		} else {
//				printf("Failed to format time\n");
//		}
//	if (xSemaphoreTake(xSemaphore_USBDataIn, portMAX_DELAY) == pdTRUE) 
//		USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)("Hello"), sizeof("Hello"));
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartResponseTask(void const * argument)
{
  extern uint16_t grambuff_usb[];
	extern __IO bool ramindex;
	extern __IO uint32_t rxlen;
	extern nex_usb_des des;
  for(;;)
  { 
		if (xSemaphoreTake(xSemaphore_USBDataOut, portMAX_DELAY) == pdTRUE) {
			// USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)grambuff_usb + (1-ramindex)*1024, rxlen);
		}
	}
}

void ReportI2c(nex_i2c_request *i2c_request)
{
	HAL_StatusTypeDef status;
	uint32_t errorCode;
	LOGData uI2c = {0};
	
	uI2c.timestamp = HAL_GetTick();
	uI2c.len = 0;
	uI2c.iserr = 0;
	uI2c.type = PROTOCOL_I2C;
	if(i2c_request->dataReadLength <= (64-8) && i2c_request->dataWriteLength <= (64-8))
	{
		if(i2c_request->dataReadLength == 0 && i2c_request->dataWriteLength != 0)//Write Only
		{
			status = HAL_I2C_Master_Transmit(&hi2c1, i2c_request->deviceAddress << 1, i2c_request->dataWriteBuffer, i2c_request->dataWriteLength, i2c_request->timeout);
			
			if (status != HAL_OK) {
				errorCode = HAL_I2C_GetError(&hi2c1);
				uI2c.len = 2;
				uI2c.iserr = 1;
				uI2c.data[0] = errorCode & 0xFF;
				uI2c.data[1] = errorCode >> 8;
			}
		}
		else if(i2c_request->dataReadLength != 0 && i2c_request->dataWriteLength == 0)//Read Only
		{
			HAL_I2C_Master_Receive(&hi2c1, i2c_request->deviceAddress << 1, (uint8_t*)uI2c.data, i2c_request->dataReadLength, i2c_request->timeout);
			if (status != HAL_OK) {
				errorCode = HAL_I2C_GetError(&hi2c1);
				uI2c.len = 2;
				uI2c.iserr = 1;
				uI2c.data[0] = errorCode & 0xFF;
				uI2c.data[1] = errorCode >> 8;
			}
			else
				uI2c.len = i2c_request->dataReadLength;
		}
		else if(i2c_request->dataReadLength != 0 && i2c_request->dataWriteLength != 0)//WriteRead
		{
			status = HAL_I2C_Master_Transmit(&hi2c1, i2c_request->deviceAddress << 1, i2c_request->dataWriteBuffer, i2c_request->dataWriteLength, i2c_request->timeout);
			//dbmsg("%X,%d,%d",i2c_request->deviceAddress << 1, i2c_request->dataWriteLength, i2c_request->timeout);
			
			if (status != HAL_OK) {
				errorCode = HAL_I2C_GetError(&hi2c1);
				uI2c.len = 2;
				uI2c.iserr = 1;
				uI2c.data[0] = errorCode & 0xFF;
				uI2c.data[1] = errorCode >> 8;
				USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
				//dbmsg("Write Fail");
				return;
			}
			HAL_I2C_Master_Receive(&hi2c1, i2c_request->deviceAddress << 1, (uint8_t*)uI2c.data, i2c_request->dataReadLength, i2c_request->timeout);
			if (status != HAL_OK) {
				errorCode = HAL_I2C_GetError(&hi2c1);
				uI2c.len = 2;
				uI2c.iserr = 1;
				uI2c.data[0] = errorCode & 0xFF;
				uI2c.data[1] = errorCode >> 8;
			}
			else
				uI2c.len = i2c_request->dataReadLength;
		}
		else
		{
				uI2c.len = 2;
				uI2c.iserr = 1;
				uI2c.data[0] = HAL_I2C_ERROR_SIZE & 0xFF;
				uI2c.data[1] = HAL_I2C_ERROR_SIZE >> 8;
		}
	}
	else
	{
			uI2c.len = 2;
			uI2c.iserr = 1;
			uI2c.data[0] = HAL_I2C_ERROR_SIZE & 0xFF;
			uI2c.data[1] = HAL_I2C_ERROR_SIZE >> 8;
	}
	USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
}

//uint8_t testBuff[400];
void StartI2cTask(void const * argument)
{
	nex_i2c_request i2c_request;
  for(;;)
  { 
		if (xQueueReceive(xQueue_I2c, &i2c_request, portMAX_DELAY) == pdTRUE) {
			ReportI2c(&i2c_request);
		}
	}
}

void PutUartData(TxRxMode Dir, uint8_t *RawData, uint16_t Size)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	LOGData uData = {0};

	if (xQueueIsQueueFullFromISR(xQueue_UartData) != pdFALSE) {
			if (xQueueReceiveFromISR(xQueue_UartData, &uData, 0) == pdPASS) {

			}
	}
	for(int i = 0;i<((Size/(sizeof(LOGData)-8)) + 1);i++)
	{
		int len = Size - i*(sizeof(LOGData)-8);
		if(len>(sizeof(LOGData)-8))
			len = (sizeof(LOGData)-8);
		uData.timestamp = HAL_GetTick();
		uData.len = len;
		uData.type = PROTOCOL_UART;
		uData.dir = Dir;
		uData.iserr = 0;
		memset(uData.data,0,(sizeof(LOGData)-8));
		memcpy(uData.data, RawData + (sizeof(LOGData)-8)*i, len);
		
		// HAL_UART_Transmit(&huart1, uData.data, len,0xffff);
		if (xQueueSendFromISR(xQueue_UartData, &uData, &xHigherPriorityTaskWoken) != pdPASS) {
				// 队列满的处理逻辑（可选）
			dbmsg("xQueueSendErr:%ld", xHigherPriorityTaskWoken);
		}
		
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void ReportUart(nex_uart_request *uart_request)
{
	LOGData uUart;
	if(uart_request->dir == Rx)
	{
		if(uxQueueMessagesWaiting(xQueue_UartData) == 0)
		{
			uUart.timestamp = HAL_GetTick();
			uUart.len = 0;
			uUart.iserr = 0;
			uUart.type = PROTOCOL_UART;
			USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uUart), sizeof(uUart));
		}
		else while (xQueueReceive(xQueue_UartData, &uUart, 10) == pdTRUE) {
			
			if(uxQueueMessagesWaiting(xQueue_UartData) == 0)
				uUart.iscontinue = 0;
			else
				uUart.iscontinue = 1;
				
			USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uUart), sizeof(uUart));
		}
	}
	else
	{
		HAL_UART_Transmit_DMA(&huart1, (uint8_t *)uart_request->dataWriteBuffer, uart_request->dataWriteLength);
		PutUartData(Tx, uart_request->dataWriteBuffer, uart_request->dataWriteLength);
	}
}

void StartUartTask(void const * argument)
{
	nex_uart_request uart_request;
  for(;;)
  { 
		if (xQueueReceive(xQueue_Uart, &uart_request, portMAX_DELAY) == pdTRUE) {
			ReportUart(&uart_request);
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
  if(&huart1 == huart)
  {
		HAL_UART_Transmit_DMA(&huart1, (uint8_t *)Uart_Recv1_Buf, Size);
		PutUartData(Rx, Uart_Recv1_Buf, Size);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart_Recv1_Buf, Uart_Max_Length);
  }
}

/* USER CODE END Application */
