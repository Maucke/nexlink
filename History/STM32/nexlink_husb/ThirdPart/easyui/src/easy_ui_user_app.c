/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#include "easy_ui_user_app.h"
#include "usbd_nex_link.h"
#include "lv_anim_light.h"
#include "usbd_nex_link.h"
#include "zf_common_font.h"
#include "animation.h"
#include "adc.h"
#include "rx8900.h"
#include "beep.h"
#include "bmp280.h"
#include <time.h>
// Pages
EasyUIPage_t pageMain, pageUSBForm, pageDialog, pageSensor, pageAnimation, pageSetting, pageAbout;

// Items
EasyUIItem_t itemUSBForm;
EasyUIItem_t itemDebug;
EasyUIItem_t itemSensor;
EasyUIItem_t itemAnimation;
EasyUIItem_t itemSetting, itemColor, itemReset, itemBrightness, titleSetting;
EasyUIItem_t itemAbout;
EasyUIItem_t itemMind, itemCircle, itemSnowflake, itemMeteo, itemPlanet, itemTriangle, itemStarwar, itemBlast, itemGCircle, itemFirework, titleAnimation;


typedef struct
{
  nex_brightness_des brides;
  nex_screen_des scrdes;
} eeprom_data;

typedef struct
{
  bool enMind;
  bool enCircle;
  bool enSnowflake;
  bool enMeteo;
  bool enPlanet;
  bool enTriangle;
  bool enStarwar;
  bool enGCircle;
  bool enFirework;
  bool reserved;
} motion_status;

motion_status mt;

extern lv_anim_t anim_backlight;
extern nex_usb_des des;
float setting_brightness;
__IO bool usbinhibit = true;
__IO bool jump2winform = false;

EasyKey_t keyUp, keyDown;
extern __IO bool menuisvisible;
extern lv_anim_t anim_beep;

void EasyUIShutDown()
{
  lv_anim_start(&anim_backlight, 0, 1000);
}

void dbug_clear(EasyUIPage_t* page);
/*!
 * @brief   Sync the operation bool value
 *
 * @param   void
 * @return  void
 */
void EasyUIKeyActionMonitor() //Interrupt trigger, No HAL_Delay(xx)
{
  extern bool mpu_left, mpu_right, mpu_ok, mpu_quit;
  if(keyUp.isPressed)
  {
    dbmsg("keyUp:isPressed");
    lv_anim_start(&anim_beep, 1000, 200);
    dbusbmsg("keyUp:isPressed");
    if(des.brides.brightness == 0)
    {
      lv_anim_start(&anim_backlight, des.brides.brightness ? des.brides.brightness : 300, 2000);
      return;
    }

    menuisvisible = !menuisvisible;
    if(menuisvisible)
      mpu_left = mpu_right = mpu_ok = mpu_quit = 0;
  }
  else if(keyUp.isHold)
  {
    dbmsg("keyUp:isHold");
    lv_anim_start(&anim_beep, 2000, 500);
    dbusbmsg("keyUp:holdTime:%d", keyUp.holdTime);
    if(menuisvisible)
      jump2winform = true;
  }
  else if(keyUp.holdTime > 5000)
  {
    dbmsg("keyUp:holdTime:%d", keyDown.holdTime);
  }
  if(keyDown.isPressed)
  {
    dbmsg("keyDown:isPressed");
    dbug_clear(&pageDialog);
    dbmsg("dialog has been clear");
  }
  else if(keyDown.isHold)
  {
    dbmsg("keyDown:isHold");
    lv_anim_start(&anim_beep, 2000, 500);
    dbusbmsg("keyDown:holdTime:%d", keyDown.holdTime);
    lv_anim_start(&anim_backlight, 0, 1000);
  }
  else if(keyDown.holdTime > 5000)
  {
    dbmsg("keyDown:holdTime:%d", keyDown.holdTime);
    HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_RESET);
  }

  if(!menuisvisible)
    return;
  if(opnForward || opnBackward || opnEnter || opnExit || opnUp || opnDown)
    return;

  opnEnter = mpu_ok;
  opnExit = mpu_quit;
  opnUp = mpu_right;
  opnDown = mpu_left;
  opnForward = mpu_left;
  opnBackward = mpu_right;

  if(opnForward || opnBackward || opnEnter || opnExit || opnUp || opnDown)
    ClearRemind();

  mpu_left = mpu_right = mpu_ok = mpu_quit = 0;
#if 0
  if(opnEnter != 0)
    dbmsg("opnEnter:%d", opnEnter);
  if(opnExit != 0)
    dbmsg("opnExit:%d", opnExit);
  if(opnUp != 0)
    dbmsg("opnUp:%d", opnUp);
  if(opnDown != 0)
    dbmsg("opnDown:%d", opnDown);
#endif
}

#define DIALOGITEMSIZE 200

EasyUIItem_t itemDialog[DIALOGITEMSIZE];
char* itemDialogStr[DIALOGITEMSIZE];
int itemDialogIndex = 0;
int itemDialogCount = 0;

typedef enum
{
  normal = 0,
  warn,
  error
} MsgType_e;

int dbug_printf(const char* pcFormat, ...)
{
  uint8_t debug_buf[DEBUG_BUF_SIZE];
  va_list args;
  int len = 0;
  memset(debug_buf, 0, sizeof debug_buf);
  va_start(args, pcFormat);

  len = vsnprintf((char*)debug_buf, sizeof(debug_buf), pcFormat, args);

  if(itemDialogStr[itemDialogIndex] != NULL)
  {
    free(itemDialogStr[itemDialogIndex]);
    itemDialogStr[itemDialogIndex] = NULL;
  }
  itemDialogStr[itemDialogIndex] = (char*)malloc(len + 1);
  if(itemDialogStr[itemDialogIndex] == NULL)
  {
    int lastItemDialogCount = itemDialogCount;
    dbug_clear(&pageDialog);
    dbmsg("dialog overflow-%d", lastItemDialogCount);
    dbmsg("dialog has been clear");//watchout the recursion
    return -1;
  }
  memcpy(itemDialogStr[itemDialogIndex], debug_buf, len);
  itemDialogStr[itemDialogIndex][len] = 0;
  if(itemDialogCount < DIALOGITEMSIZE)
  {
    if(itemDialogCount & 1)
      EasyUIAddItem(&pageDialog, &itemDialog[itemDialogIndex], "", ITEM_DETAIL);
    else
      EasyUIAddItem(&pageDialog, &itemDialog[itemDialogIndex], "", ITEM_DETAIL);

    itemDialog[itemDialogIndex].msg = itemDialogStr[itemDialogIndex];
    itemDialog[itemDialogIndex].title = itemDialog[itemDialogIndex].msg + 10; //sizeof [000.000]-
  }
  else
  {
    for(int i = 0; i < DIALOGITEMSIZE - 1; i++)
    {
      itemDialog[i].msg = itemDialog[i + 1].msg;
      itemDialog[i].title = itemDialog[i].msg + 10;
    }
    itemDialog[DIALOGITEMSIZE - 1].msg = itemDialogStr[itemDialogIndex];
    itemDialog[DIALOGITEMSIZE - 1].title = itemDialog[itemDialogIndex].msg + 10;
  }

  itemDialogIndex = (itemDialogIndex + 1) % DIALOGITEMSIZE;
  if(itemDialogCount < DIALOGITEMSIZE)
    itemDialogCount++;
  va_end(args);

  return len;
}

void dbug_clear(EasyUIPage_t* page)
{
  itemDialogIndex = 0;
  itemDialogCount = 0;
  page->itemHead = NULL;
  page->itemTail = NULL;
  for(int i = 0; i < DIALOGITEMSIZE; i++)
    if(itemDialogStr[itemDialogIndex] != NULL)
    {
      free(itemDialogStr[itemDialogIndex]);
      itemDialogStr[itemDialogIndex] = NULL;
    }
}

void EventJump()
{
  if(jump2winform)
  {
    jump2winform = false;
    EasyUIItemOperationResponse(&pageUSBForm, &itemUSBForm, &itemUSBForm.id);
  }
}

void EventMotion()
{
  if(mt.enMind)
    Motion_Mind();
  if(mt.enCircle)
    Motion_Circle();
  if(mt.enSnowflake)
    Motion_Snowflake();
  if(mt.enMeteo)
    Motion_Movmeteor();
  if(mt.enPlanet)
    Motion_Planet();
  if(mt.enTriangle)
    Motion_Triangle();
  if(mt.enStarwar)
    Motion_StarWar();
  if(mt.enGCircle)
    Motion_GCFireworks();
  if(mt.enFirework)
    Motion_Firework();
}

void EventChangeBrightness(EasyUIItem_t* item)
{
  if(opnUp)
  {
    if(*item->param + 10 <= 100)
      *item->param += 10;
    else
      *item->param = 100;
    des.brides.brightness =  *item->param == 100 ? 999 : (*item->param) * 10;
    lv_anim_start(&anim_backlight, des.brides.brightness, 100);
    opnUp = opnForward = false;
  }
  if(opnDown)
  {
    if(*item->param - 10 >= 10)
      *item->param -= 10;
    else
      *item->param = 10;
    des.brides.brightness =  *item->param == 100 ? 999 : (*item->param) * 10;
    lv_anim_start(&anim_backlight, des.brides.brightness, 100);
    opnDown = opnBackward = false;
  }

  if(opnEnter)
  {
    item->paramBackup = *item->param;
    EasyUIBackgroundBlur();
    functionIsRunning = false;
    opnEnter = false;
  }
  if(opnExit)
  {
    *item->param = item->paramBackup;
    des.brides.brightness =  *item->param == 100 ? 999 : (*item->param) * 10;
    lv_anim_start(&anim_backlight, des.brides.brightness, 100);
    EasyUIBackgroundBlur();
    functionIsRunning = false;
    opnExit = false;
  }
  // Clear the states of key to monitor next key action
  opnForward = opnBackward = opnEnter = opnUp = opnDown = false;
}

void PageAbout(EasyUIPage_t* page)
{
  char tempstr[128];
  int screen_delta = 10;

  EasyUIDisplayStr(10, screen_delta, "MCU: STM32F405RG");
  screen_delta += page->rowheight;
  EasyUIDisplayStr(10, screen_delta, "MPU: MPU6050");
  screen_delta += page->rowheight;
  EasyUIDisplayStr(10, screen_delta, "CLOCK: RX8900");
  screen_delta += page->rowheight;
  EasyUIDisplayStr(10, screen_delta, "SEN: BMP280");
  screen_delta += page->rowheight;
  EasyUIDisplayStr(10, screen_delta, "Author: DPJ");
  screen_delta += page->rowheight;
  EasyUIDisplayStr(10, screen_delta, EasyUIVersion);
  screen_delta += page->rowheight;
  snprintf(tempstr, sizeof tempstr, "Rel. %s", __DATE__);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
}

void PageUSBForm(EasyUIPage_t* page)
{
  if(usbinhibit)
  {
    EasyUITransitionAnim();
    EasyUIClearBuffer();
    EasyUISendBuffer();
    usbinhibit = false;
  }
  if(opnExit)
  {
    usbinhibit = true;
		
		NV3030B_SetRotation(0);
  }
}

void lowBatteryAction()
{
  float battery = Get_Battery_Value();
  if(battery < 3.2f)
    lv_anim_start(&anim_backlight, 0, 100);
}

int batteryVoltageToPercentage(float voltage)
{
  float minVoltage = 3.0f;
  float maxVoltage = 4.2f;

  // 计算电压在范围内的百分比
  if(voltage < minVoltage)
  {
    return 0; // 如果电压低于最小值，返回0%
  }
  else if(voltage > maxVoltage)
  {
    return 100; // 如果电压高于最大值，返回100%
  }
  else
  {
    // 在最小值和最大值之间进行线性插值计算
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;
    return (int)percentage;
  }
}

static Filter fltaltitude;
static Filter fltvoltagex100;

void PageSensor(EasyUIPage_t* page)
{
  static int levelrun = 0;
  char tempstr[64];
  int screen_delta = 10;
  static struct tm time_user;
  static float pressure, temperature, humidity, asl;
  static long last_update_time = 0;
  long now_tick = HAL_GetTick();
  if(now_tick - last_update_time > 500)
  {
    fltvoltagex100.target_value = Get_Battery_Value() * 100;
    update_filter(&fltvoltagex100);

    if(HAL_GPIO_ReadPin(PW_CHARGE_GPIO_Port, PW_CHARGE_Pin) == GPIO_PIN_RESET)
      levelrun = (levelrun + 1) % 4;
    else if(BatteryVoltage_To_Level(fltvoltagex100.current_value / 100.0f) != -1)
      levelrun = BatteryVoltage_To_Level(fltvoltagex100.current_value / 100.0f);
    else
      levelrun = levelrun ? 0 : 1;
    RX8900_GetTime(&time_user);
    BMP280_GetData(&pressure, &temperature, &humidity, &asl);
    fltaltitude.target_value = BMP280_PressureToAltitude(&pressure);
    update_filter(&fltaltitude);

    last_update_time = HAL_GetTick();
  }


  snprintf(tempstr, sizeof tempstr, "BAT: %.1f V", fltvoltagex100.current_value / 100.0f);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
  if(HAL_GPIO_ReadPin(PW_CHARGE_GPIO_Port, PW_CHARGE_Pin) == GPIO_PIN_RESET)
    snprintf(tempstr, sizeof tempstr, "CHARGING");
  else
    snprintf(tempstr, sizeof tempstr, "DISCHARGE");
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
  NV3030B_DrawBMP565(165, screen_delta, 36, 36, gImage_Battery[levelrun]);

  snprintf(tempstr, sizeof tempstr, "%04d-%02d-%02d, %s", time_user.tm_year + 1900, time_user.tm_mon + 1, time_user.tm_mday, weekdays[time_user.tm_wday % 7]);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
  snprintf(tempstr, sizeof tempstr, "%02d:%02d:%02d", time_user.tm_hour % 100, time_user.tm_min % 100, time_user.tm_sec % 100);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;

  snprintf(tempstr, sizeof tempstr, "P: %.1f Pa", pressure);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
  snprintf(tempstr, sizeof tempstr, "T: %.1f C", temperature);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
  snprintf(tempstr, sizeof tempstr, "Alt: %.1f M", fltaltitude.current_value);
  EasyUIDisplayStr(10, screen_delta, tempstr);
  screen_delta += page->rowheight;
}
extern USBD_HandleTypeDef hUSB;
extern bool usbavaliable;

void PageDialog(EasyUIItem_t* page)
{

}

void MenuInit()
{
  setting_brightness = (des.brides.brightness + 1) / 10;
  mt.enFirework = true;
  mt.enStarwar = true;
  EasyUIAddPage(&pageMain, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_LIST);
  EasyUIAddPage(&pageSetting, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_LIST);
  EasyUIAddPage(&pageUSBForm, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_CUSTOM, PageUSBForm);
  EasyUIAddPage(&pageDialog, NV3030B_6X8_FONT, PAGE_LIST);
  EasyUIAddPage(&pageSensor, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_CUSTOM, PageSensor);
  EasyUIAddPage(&pageAnimation, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_LIST);
  EasyUIAddPage(&pageAbout, NV3030B_DEFAULT_DISPLAY_FONT, PAGE_CUSTOM, PageAbout);

  EasyUIAddItem(&pageMain, &itemUSBForm, "USBForm", ITEM_JUMP_PAGE, pageUSBForm.id);
  EasyUIAddItem(&pageMain, &itemDebug, "Debug", ITEM_JUMP_PAGE, pageDialog.id);
  EasyUIAddItem(&pageMain, &itemSensor, "Sensor", ITEM_JUMP_PAGE, pageSensor.id);
  EasyUIAddItem(&pageMain, &itemAnimation, "Animation", ITEM_JUMP_PAGE, pageAnimation.id);
  EasyUIAddItem(&pageMain, &itemSetting, "Setting", ITEM_JUMP_PAGE, pageSetting.id);
  EasyUIAddItem(&pageMain, &itemAbout, "<About>", ITEM_JUMP_PAGE, pageAbout.id);

  EasyUIAddItem(&pageSetting, &titleSetting, "[Setting]", ITEM_PAGE_DESCRIPTION);
  EasyUIAddItem(&pageSetting, &itemColor, "Reversed", ITEM_SWITCH, &reversedColor);
  EasyUIAddItem(&pageSetting, &itemBrightness, "Brightness", ITEM_PROGRESS_BAR, &setting_brightness, EventChangeBrightness);

  EasyUIAddItem(&pageAnimation, &titleAnimation, "[Animation]", ITEM_PAGE_DESCRIPTION);
  EasyUIAddItem(&pageAnimation, &itemMind, "Mind", ITEM_CHECKBOX, &mt.enMind);
  EasyUIAddItem(&pageAnimation, &itemCircle, "Circle", ITEM_CHECKBOX, &mt.enCircle);
  EasyUIAddItem(&pageAnimation, &itemSnowflake, "Snowflake", ITEM_CHECKBOX, &mt.enSnowflake);
  EasyUIAddItem(&pageAnimation, &itemMeteo, "Meteo", ITEM_CHECKBOX, &mt.enMeteo);
  EasyUIAddItem(&pageAnimation, &itemPlanet, "Planet", ITEM_CHECKBOX, &mt.enPlanet);
  EasyUIAddItem(&pageAnimation, &itemTriangle, "Triangle", ITEM_CHECKBOX, &mt.enTriangle);
  EasyUIAddItem(&pageAnimation, &itemStarwar, "Starwar", ITEM_CHECKBOX, &mt.enStarwar);
  EasyUIAddItem(&pageAnimation, &itemGCircle, "GCircle", ITEM_CHECKBOX, &mt.enGCircle);
  EasyUIAddItem(&pageAnimation, &itemFirework, "Firework", ITEM_CHECKBOX, &mt.enFirework);
  dbusbmsg("setting_brightness: %f", setting_brightness);

//		EasyUIItemOperationResponse(&pageAnimation, &itemAnimation, &itemAnimation.id);
  Motion_Init();
  // Key init
  EasyKeyInit(&keyUp, GPIOB, GPIO_PIN_6);
  EasyKeyInit(&keyDown, GPIOB, GPIO_PIN_7);
}
