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
QueueHandle_t xQueue_I2c;
osThreadId responseTaskHandle;
osThreadId I2cTaskHandle;
SemaphoreHandle_t xSemaphore_USB;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartResponseTask(void const * argument);
void StartI2cTask(void const * argument);
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
	xQueue_Uart = xQueueCreate( QUEUE_MAX_SIZE,  QUEUE_LOG_SIZE);
	if( xQueue_Uart == NULL )
	{
			/* Queue was not created and must not be used. */
	}
	dbmsg("size:%d", sizeof(nex_i2c_request));
	xQueue_I2c = xQueueCreate( 1,  sizeof(nex_i2c_request));
	if( xQueue_I2c == NULL )
	{
			/* Queue was not created and must not be used. */
	}
	xSemaphore_USB = xSemaphoreCreateBinary();
	if( xSemaphore_USB == NULL )
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
		if (xSemaphoreTake(xSemaphore_USB, portMAX_DELAY) == pdTRUE) {
			// USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)grambuff_usb + (1-ramindex)*1024, rxlen);
		}
	}
}
void StartI2cTask(void const * argument)
{
	HAL_StatusTypeDef status;
	uint32_t errorCode;
	nex_i2c_request i2c_request;
	LOGData uI2c;
  for(;;)
  { 
		if (xQueueReceive(xQueue_I2c, &i2c_request, portMAX_DELAY) == pdTRUE) {
			uI2c.timestamp = HAL_GetTick();
			uI2c.len = 0;
			uI2c.iserr = 0;
			uI2c.type = PROTOCOL_I2C;
			if(i2c_request.dataReadLength <= (64-8) && i2c_request.dataWriteLength <= (64-8))
			{
				if(i2c_request.dataReadLength == 0 && i2c_request.dataWriteLength != 0)//Write Only
				{
					status = HAL_I2C_Master_Transmit(&hi2c1, i2c_request.deviceAddress << 1, i2c_request.dataWriteBuffer, i2c_request.dataWriteLength, i2c_request.timeout);
					
					if (status != HAL_OK) {
						errorCode = HAL_I2C_GetError(&hi2c1);
						uI2c.len = 2;
						uI2c.iserr = 1;
						uI2c.data[0] = errorCode & 0xFF;
						uI2c.data[1] = errorCode >> 8;
					}
					USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
				}
				else if(i2c_request.dataReadLength != 0 && i2c_request.dataWriteLength == 0)//Read Only
				{
					HAL_I2C_Master_Receive(&hi2c1, i2c_request.deviceAddress << 1, (uint8_t*)uI2c.data, i2c_request.dataReadLength, i2c_request.timeout);
					if (status != HAL_OK) {
						errorCode = HAL_I2C_GetError(&hi2c1);
						uI2c.len = 2;
						uI2c.iserr = 1;
						uI2c.data[0] = errorCode & 0xFF;
						uI2c.data[1] = errorCode >> 8;
					}
					else
						uI2c.len = i2c_request.dataReadLength;
					USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
				}
				else if(i2c_request.dataReadLength != 0 && i2c_request.dataWriteLength != 0)//WriteRead
				{
					status = HAL_I2C_Master_Transmit(&hi2c1, i2c_request.deviceAddress << 1, i2c_request.dataWriteBuffer, i2c_request.dataWriteLength, i2c_request.timeout);
					//dbmsg("%X,%d,%d",i2c_request.deviceAddress << 1, i2c_request.dataWriteLength, i2c_request.timeout);
					
					if (status != HAL_OK) {
						errorCode = HAL_I2C_GetError(&hi2c1);
						uI2c.len = 2;
						uI2c.iserr = 1;
						uI2c.data[0] = errorCode & 0xFF;
						uI2c.data[1] = errorCode >> 8;
						USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
						//dbmsg("Write Fail");
						continue;
					}
					HAL_I2C_Master_Receive(&hi2c1, i2c_request.deviceAddress << 1, (uint8_t*)uI2c.data, i2c_request.dataReadLength, i2c_request.timeout);
					if (status != HAL_OK) {
						errorCode = HAL_I2C_GetError(&hi2c1);
						uI2c.len = 2;
						uI2c.iserr = 1;
						uI2c.data[0] = errorCode & 0xFF;
						uI2c.data[1] = errorCode >> 8;
					}
					else
						uI2c.len = i2c_request.dataReadLength;
					// printf("Ret:%d, I2c:%02X %02X %02X %02X %02X %02X %02X %02X \n",status, uI2c.data[0], uI2c.data[1], uI2c.data[2], uI2c.data[3], uI2c.data[4], uI2c.data[5], uI2c.data[6], uI2c.data[7]);
					USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
				}
				else
				{
						uI2c.len = 2;
						uI2c.iserr = 1;
						uI2c.data[0] = HAL_I2C_ERROR_SIZE & 0xFF;
						uI2c.data[1] = HAL_I2C_ERROR_SIZE >> 8;
						USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
				}
			}
			else
			{
					uI2c.len = 2;
					uI2c.iserr = 1;
					uI2c.data[0] = HAL_I2C_ERROR_SIZE & 0xFF;
					uI2c.data[1] = HAL_I2C_ERROR_SIZE >> 8;
					USBD_NEX_LINK_Transmit(&hUSB, (uint8_t *)(&uI2c), sizeof(uI2c));
			}
		}
	}
}
//#define HAL_I2C_ERROR_NONE              0x00000000U    /*!< No error              */
//#define HAL_I2C_ERROR_BERR              0x00000001U    /*!< BERR error            */
//#define HAL_I2C_ERROR_ARLO              0x00000002U    /*!< ARLO error            */
//#define HAL_I2C_ERROR_AF                0x00000004U    /*!< AF error              */
//#define HAL_I2C_ERROR_OVR               0x00000008U    /*!< OVR error             */
//#define HAL_I2C_ERROR_DMA               0x00000010U    /*!< DMA transfer error    */
//#define HAL_I2C_ERROR_TIMEOUT           0x00000020U    /*!< Timeout Error         */
//#define HAL_I2C_ERROR_SIZE              0x00000040U    /*!< Size Management error */
//#define HAL_I2C_ERROR_DMA_PARAM         0x00000080U    /*!< DMA Parameter Error   */
//#define HAL_I2C_WRONG_START             0x00000200U    /*!< Wrong start Error     */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
	LOGData uData;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if(&huart1 == huart)
  {
    // 检查队列是否已满
    if (xQueueIsQueueFullFromISR(xQueue_Uart) != pdFALSE) {
        if (xQueueReceiveFromISR(xQueue_Uart, &uData, 0) == pdPASS) {

        }
    }
		for(int i = 0;i<((Size/(QUEUE_LOG_SIZE-8)) + 1);i++)
		{
			int len = Size - i*(QUEUE_LOG_SIZE-8);
			if(len>(QUEUE_LOG_SIZE-8))
				len = (QUEUE_LOG_SIZE-8);
			uData.timestamp = HAL_GetTick();
			uData.len = len;
			uData.type = PROTOCOL_UART;
			uData.dir = Rx;
			uData.iserr = 0;
			memset(uData.data,0,(QUEUE_LOG_SIZE-8));
			memcpy(uData.data, Uart_Recv1_Buf + (QUEUE_LOG_SIZE-8)*i, len);
			
			// HAL_UART_Transmit(&huart1, uData.data, len,0xffff);
			if (xQueueSendFromISR(xQueue_Uart, &uData, &xHigherPriorityTaskWoken) != pdPASS) {
					// 队列满的处理逻辑（可选）
				dbmsg("xQueueSendErr:%d", xHigherPriorityTaskWoken);
			}
			
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart_Recv1_Buf, Uart_Max_Length);
  }
}

/* USER CODE END Application */
