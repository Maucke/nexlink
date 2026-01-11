#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "stm32f4xx_hal.h"
#include "nv3030b.h"
#include "stdlib.h"

#define AniScreenInit(gram)                                  (NV3030B_Init(gram))
#define AniDisplayStr(x, y, str)                             (NV3030B_ShowStr(x, y, str))
#define AniDisplayFloat(x, y, dat, num, pointNum)            (NV3030B_ShowFloat(x, y, dat, num, pointNum))
#define AniDrawDot(x, y, color)                              (NV3030B_DrawPoint(x, y, color))
#define AniDrawLine(x, y, x_end, y_end, color)               (NV3030B_DrawLine(x, y, x_end, y_end, color))
#define AniDrawBox(x, y, width, height, color)               (NV3030B_DrawBox(x, y, width, height, color))
#define AniDrawFrame(x, y, width, height, color)             (NV3030B_DrawFrame(x, y, width, height, color))
#define AniDrawRFrame(x, y, width, height, color, r)         (NV3030B_DrawRFrame(x, y, width, height, color, r))
#define AniDrawRBox(x, y, width, height, color, r)           (NV3030B_DrawRBox(x, y, width, height, color, r))
#define AniClearBuffer(void)                                     (NV3030B_ClearBuffer())
#define AniSendBuffer(void)                                      (NV3030B_SendBuffer())
#define AniSetDrawColor(mode)                                (NV3030B_SetDrawColor(mode))
#define AniDisplayBMP(x, y, width, height, pic)              (NV3030B_ShowBMP(x, y, width, height, pic))
#define AniModifyColor(void)                                     (NV3030B_ModifyColor())
#define AniDrawCircle(x, y, r, color, section)                 (NV3030B_DrawCircle(x, y, r, color, section))
#define AniDrawDisc(x, y, r, color, section)                 (NV3030B_DrawDisc(x, y, r, color, section))
#define AniFillTriangle(x0, y0, x1, y1, x2, y2, color)                 (NV3030B_FillTriangle(x0, y0, x1, y1, x2, y2, color))
#define AniTriangle(x0, y0, x1, y1, x2, y2, color)                 (NV3030B_Triangle(x0, y0, x1, y1, x2, y2, color))

#define SCR_WIDTH LCD_W
#define SCR_HEIGHT LCD_H
#define PI 3.1415926f
#define MINDMAX 14
#define GCIRCLEMAX 6
#define CIRCLEMAX 8
#define STARMAX 20
#define METEORMAX 20
#define DEFLINEMAX 30
#define PLANETMAX 5
#define TRIANGLEMAX 8
#define FULLTRIANGLEMAX 8
#define STARWARMAX 15
/***星空穿越特效相关参数*/
#define FACTOR_SIZE 1  //星空粒子大小//方形时为方形的边长//圆形时为圆形半径
#define FACTOR_SHAPE 1 //星空粒子形状//0 方形//1 圆形

typedef enum
{
	OK = 0x00U,
	ERR = 0x01U,
	BUSY = 0x02U,
	IDLE = 0x03U,
} ANI_STATUS;

#ifdef __cplusplus
extern "C"
{
#endif

void Motion_MindInit(void);
void Motion_Mind(void);
void Motion_CircleInit(void);
void Motion_Circle(void);
void Motion_SnowflakeInit(void);
void Motion_Snowflake(void);
void Motion_MovmeteorInit(void);
void Motion_Movmeteor(void);
void Motion_PlanetInit(void);
void Motion_Planet(void);
void Motion_TriangleInit(void);
void Motion_Triangle(void);
void Motion_TriangleF(void);
void Motion_StarWarInit(void);
void Motion_StarWar(void);
void Motion_GCFireworksInit(void);
void Motion_GCFireworks(void);
void Motion_FireworkInit(void);
void Motion_Firework(void);;

void Motion_Init(void);

ANI_STATUS MovMind(uint8_t Index);
ANI_STATUS FucCircle(uint8_t Index);
ANI_STATUS MovSnowflake(uint8_t Index);
ANI_STATUS Movmeteor(uint8_t Index);
ANI_STATUS Planet(uint8_t Index);
ANI_STATUS Triangle(uint8_t Index);
ANI_STATUS MovStarWar(uint8_t Index);
ANI_STATUS GCFireworks(uint8_t Index);

#ifdef __cplusplus
}
#endif

#endif
