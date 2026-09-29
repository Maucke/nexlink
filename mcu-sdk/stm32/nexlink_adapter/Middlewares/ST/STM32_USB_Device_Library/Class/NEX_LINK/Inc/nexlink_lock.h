#pragma once
#include <stdint.h>
#include "stm32f4xx.h"     /* __get_IPSR() 等 CMSIS intrinsic */
#include "FreeRTOS.h"
#include "task.h"

/*
 * NEX_LINK 的临界区。
 *
 * 内存池标记和发送队列会被任务和中断同时访问 —— adapter 的
 * HAL_UARTEx_RxEventCallback() 就在中断里 send_event()，而 nexlink_tx_send()
 * 又会立刻泵一次队列，所以这一整条链都可能跑在中断上下文里。
 *
 * 必须按上下文选进入方式：在中断里调 taskENTER_CRITICAL() 会被
 * vPortEnterCritical() 里的
 *     configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 )
 * 拦下来，而本工程的 configASSERT 是 taskDISABLE_INTERRUPTS() + for(;;)，
 * 直接关中断死机（症状就是卡在 buf_alloc 里出不来）。
 *
 * 用 BASEPRI 保存/恢复，两层配平，任务和中断里都可嵌套。
 */

#define NEXLINK_LOCK_IN_TASK  (0xFFFFFFFFUL)   /* BASEPRI 只用到低 8 位，不会撞 */

static inline uint32_t nexlink_lock_enter(void)
{
    if (__get_IPSR() != 0U)
        return taskENTER_CRITICAL_FROM_ISR();

    taskENTER_CRITICAL();
    return NEXLINK_LOCK_IN_TASK;
}

static inline void nexlink_lock_exit(uint32_t token)
{
    if (token != NEXLINK_LOCK_IN_TASK)
        taskEXIT_CRITICAL_FROM_ISR(token);
    else
        taskEXIT_CRITICAL();
}
