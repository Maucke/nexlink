#include "main.h"
#include "rx8900.h"
#include "stdio.h"
#include "myiic.h"

#define RX8900_ADDR  (0x32)

char weekdays[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static uint8_t B2D(uint8_t bcd);
static uint8_t D2B(uint8_t decimal);
struct tm tm_local = {
  .tm_sec = 00,
  .tm_min = 22,
  .tm_hour = 10,
  .tm_wday = 4,
  .tm_mday = 20,
  .tm_mon = 6-1,
  .tm_year = 124,
};
void RX8900_Init()
{
	IIC_Init();
  IIC_Write_Byte(RX8900_ADDR, RX8900_EXT_REG, 8);
  IIC_Write_Byte(RX8900_ADDR, RX8900_REG_STATUS, 0);
  IIC_Write_Byte(RX8900_ADDR, RX8900_REG_CONTROL, 64);
//	RX8900_SetTime(&tm_local);
}

bool RX8900_SetTime(struct tm *tm_local)
{
  uint8_t buffer[8] = {D2B(tm_local->tm_sec), D2B(tm_local->tm_min), D2B(tm_local->tm_hour), (tm_local->tm_wday), D2B(tm_local->tm_mday), D2B(tm_local->tm_mon+1), D2B(tm_local->tm_year % 100)};
  
	if(IIC_Write_Len(RX8900_ADDR, RX8900_REG_TIME, 7, buffer))
		return false;
  return true;
}

bool RX8900_GetTime(struct tm *tm_local)
{
  uint8_t buffer[7] = {0,};
	if(IIC_Read_Len(RX8900_ADDR, RX8900_REG_TIME, 7, buffer))
		return false;
		
  tm_local->tm_sec = B2D(buffer[0] & 0x7F);
  tm_local->tm_min = B2D(buffer[1] & 0x7F);
  tm_local->tm_hour = B2D(buffer[2] & 0x3F);
  tm_local->tm_wday = (buffer[3] & 0x7f);
  tm_local->tm_mday = B2D(buffer[4] & 0x3F);
  tm_local->tm_mon = B2D(buffer[5] & 0x1F)-1;
  tm_local->tm_year = B2D(buffer[6])%100 + 100;
			
  return true;
}

void RX8900_Test(int interval)
{
	struct tm tm_local;
  static long last_update_time;
  long now_tick = HAL_GetTick();
  if(now_tick - last_update_time > interval)
  {
    RX8900_GetTime(&tm_local);
    dbmsg("Data: %04d-%02d-%02d", tm_local.tm_year+1900,tm_local.tm_mon+1,tm_local.tm_mday);
    dbmsg("Week: %d", tm_local.tm_wday);
    dbmsg("Time: %02d:%02d:%02d", tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);
    last_update_time = HAL_GetTick();
  }
}

bool RX8900_ReadTemperature(float *rtctemp)
{
  uint8_t startAddr = RX8900_REG_TEMP;
  uint8_t temp = IIC_Read_Byte(RX8900_ADDR, startAddr);
	
	if(!temp)
		return false;

  *rtctemp = (temp * 2 - 187.19) / 3.218;
	return true;
}

//bool RX8900_SetAlarm1(uint8_t mode, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec)
//{
//  uint8_t alarmSecond = D2B(sec);
//  uint8_t alarmMinute = D2B(min);
//  uint8_t alarmHour = D2B(hour);
//  uint8_t alarmDate = D2B(date);

//  switch(mode)
//  {
//  case ALARM_MODE_ALL_MATCHED:
//    break;
//  case ALARM_MODE_HOUR_MIN_SEC_MATCHED:
//    alarmDate |= 0x80;
//    break;
//  case ALARM_MODE_MIN_SEC_MATCHED:
//    alarmDate |= 0x80;
//    alarmHour |= 0x80;
//    break;
//  case ALARM_MODE_SEC_MATCHED:
//    alarmDate |= 0x80;
//    alarmHour |= 0x80;
//    alarmMinute |= 0x80;
//    break;
//  case ALARM_MODE_ONCE_PER_SECOND:
//    alarmDate |= 0x80;
//    alarmHour |= 0x80;
//    alarmMinute |= 0x80;
//    alarmSecond |= 0x80;
//    break;
//  default:
//    break;
//  }

//  /* Write Alarm Registers */
//  uint8_t startAddr = RX8900_REG_ALARM1;
//  uint8_t buffer[5] = {startAddr, alarmSecond, alarmMinute, alarmHour, alarmDate};
//  if(HAL_I2C_Master_Transmit(i2c, RX8900_ADDR, buffer, sizeof(buffer), 50) != HAL_OK) return false;

//  /* Enable Alarm1 at Control Register */
//  uint8_t ctrlReg = 0x00;
//  IIC_Read_Byte(RX8900_ADDR, RX8900_REG_CONTROL, &ctrlReg);
//  ctrlReg |= RX8900_CON_A1IE;
//  ctrlReg |= RX8900_CON_INTCN;
//  IIC_Write_Byte(RX8900_ADDR, RX8900_REG_CONTROL, ctrlReg);

//  return true;
//}

bool RX8900_ClearAlarm1()
{
  uint8_t ctrlReg;
  uint8_t statusReg;

  /* Clear Control Register */
  ctrlReg = IIC_Read_Byte(RX8900_ADDR, RX8900_REG_CONTROL);
  ctrlReg &= ~RX8900_CON_A1IE;
  IIC_Write_Byte(RX8900_ADDR, RX8900_REG_CONTROL, ctrlReg);

  /* Clear Status Register */
  statusReg = IIC_Read_Byte(RX8900_ADDR, RX8900_REG_STATUS);
  statusReg &= ~RX8900_STA_A1F;
  IIC_Write_Byte(RX8900_ADDR, RX8900_REG_STATUS, statusReg);

  return true;
}

static uint8_t B2D(uint8_t bcd)
{
  return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t D2B(uint8_t decimal)
{
  return (((decimal / 10) << 4) | (decimal % 10));
}
