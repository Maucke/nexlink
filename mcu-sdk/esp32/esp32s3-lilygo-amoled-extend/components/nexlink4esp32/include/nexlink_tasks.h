#pragma once

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void NexLinkInit(void);

    /* NexLinkTask 的句柄：USB 事件回调、以及入队新帧的地方用它唤醒任务 */
    extern TaskHandle_t nexlink_task_handle;

#ifdef __cplusplus
}
#endif
