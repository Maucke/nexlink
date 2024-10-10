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
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

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
	dbmsg("UartData size:%d", sizeof( UartData ));
	xQueue_Uart = xQueueCreate( QUEUE_MAX_SIZE,  QUEUE_LOG_SIZE);
	if( xQueue_Uart == NULL )
	{
			/* Queue was not created and must not be used. */
	}
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
	UartData uData;

	struct tm tm_local;
	char time_str[32];
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
	usb_printf("Hello USB");
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  { 
		SYS_GetTime(&tm_local);
		// 格式化时间为字符串
		if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_local) != 0) {
				printf("%s\n", time_str); // 打印时间
		} else {
				printf("Failed to format time\n");
		}
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
	UartData uData;
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
			uData.reserve2 = 0;
			memset(uData.data,0,(QUEUE_LOG_SIZE-8));
			memcpy(uData.data, Uart_Recv1_Buf + (QUEUE_LOG_SIZE-8)*i, len);
			
			// HAL_UART_Transmit(&huart1, uData.data, len,0xffff);
			if (xQueueSendFromISR(xQueue_Uart, &uData, &xHigherPriorityTaskWoken) != pdPASS) {
					// 队列满的处理逻辑（可选）
				dbmsg("xQueueSendErr:%d", xHigherPriorityTaskWoken);
			}
		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart_Recv1_Buf, Uart_Max_Length);
  }
}

/* USER CODE END Application */
