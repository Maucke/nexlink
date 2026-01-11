/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#ifndef _MENU_H
#define _MENU_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>
#include "easy_ui.h"
#include "easy_key.h"

extern EasyKey_t keyUp, keyDown;           // Used to control value up and down

void MenuInit(void);
void EasyUIKeyActionMonitor(void);
void EventJump(void);
void lowBatteryAction(void);
void EasyUIShutDown(void);
	
#ifdef __cplusplus
}
#endif

#endif
