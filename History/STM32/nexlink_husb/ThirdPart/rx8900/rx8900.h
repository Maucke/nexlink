#ifndef STM32_RX8900_H_
#define STM32_RX8900_H_

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "main.h"

#define RX8900_REG_TIME         0x00
#define RX8900_REG_ALARM1       0x07
#define RX8900_REG_ALARM2       0x0B
#define RX8900_EXT_REG          0x0D
#define RX8900_REG_CONTROL      0x0F
#define RX8900_REG_STATUS       0x0E
#define RX8900_REG_TEMP         0x17

#define RX8900_CON_EOSC         0x80
#define RX8900_CON_BBSQW        0x40
#define RX8900_CON_CONV         0x20
#define RX8900_CON_RS2          0x10
#define RX8900_CON_RS1          0x08
#define RX8900_CON_INTCN        0x04
#define RX8900_CON_A2IE         0x02
#define RX8900_CON_A1IE         0x01

#define RX8900_STA_OSF          0x80
#define RX8900_STA_32KHZ        0x08
#define RX8900_STA_BSY          0x04
#define RX8900_STA_A2F          0x02
#define RX8900_STA_A1F          0x01
extern char weekdays[7][4];
typedef enum
{
  ALARM_MODE_ALL_MATCHED = 0,
  ALARM_MODE_HOUR_MIN_SEC_MATCHED,
  ALARM_MODE_MIN_SEC_MATCHED,
  ALARM_MODE_SEC_MATCHED,
  ALARM_MODE_ONCE_PER_SECOND
} AlarmMode;

typedef enum
{
  SUNDAY = 0,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY
} DaysOfWeek;


void RX8900_Init(void);
bool RX8900_GetTime(struct tm *tm_local);
bool RX8900_SetTime(struct tm *tm_local);
bool RX8900_ReadTemperature(float *rtctemp);
bool RX8900_SetAlarm1(AlarmMode mode, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
bool RX8900_ClearAlarm1(void);
void RX8900_Test(int interval);

#endif /* STM32_RX8900_H_ */
