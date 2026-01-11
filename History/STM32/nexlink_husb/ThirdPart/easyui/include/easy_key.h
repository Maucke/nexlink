/*!
 * Copyright (c) 2022, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#ifndef _easy_key_h_
#define _easy_key_h_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "main.h"

#define FILTER_TIME_MS          1     // Dithering elimination
#define UPDATE_KEY_STATE_MS     10       // Update key state once per (x) ms
#define HOLD_THRESHOLD_MS       500     // Time longer than this is considered as "hold"
#define INTERVAL_THRESHOLD_MS   240     // Trigger time interval less than this is considered as "multiClick"

typedef struct EasyKey_typedef
{
    // Internal call
    uint8_t value, cacheValue;      // Press:0  Not press:1
    uint8_t preValue;
    struct EasyKey_typedef *next;
    GPIO_TypeDef* GPIOx;
		uint16_t GPIO_Pin;
    uint32_t holdTime, intervalTime;

    enum
    {
        down,
        up,
        pressed,
        released
    } state;

    // User call
    bool isPressed;
    bool isHold;
    bool isMultiClick;
    uint8_t clickState;
} EasyKey_t;

void EasyKeyInit(EasyKey_t *key, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void EasyKeyScanKeyState(void);

extern bool multiClickSwitch;
void EasyKeyUserApp(void);

extern EasyKey_t keyL, keyC, keyR;
#endif
