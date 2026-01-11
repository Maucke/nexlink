/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 * 
 * @author  Baohan
 */

#ifndef nv3030b_h_
#define nv3030b_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "zf_common_font.h"
#include "my_math.h"
#include "zf_common_function.h"

#define NV3030B_SPI_TX_DMA DMA2_Stream3				

#define X_MAX 240				
#define Y_MAX 280

#define LCD_W X_MAX
#define LCD_H Y_MAX

typedef enum
{
	DMA_MEMINC_ENABLE = 0,
	DMA_MEMINC_DISABLE
} DMA_MEMINC_STATE;

#define NV3030B_DC_GPIO GPIOA
#define NV3030B_DC_PIN GPIO_PIN_8

#define NV3030B_CS_GPIO GPIOA
#define NV3030B_CS_PIN GPIO_PIN_4

#define NV3030B_RST_GPIO GPIOC
#define NV3030B_RST_PIN GPIO_PIN_9

#define PIN_OUT(PORT, PIN, STATUS) (PORT)->BSRR = (STATUS) ? (PIN) : (uint32_t)(PIN) << 16

#define NV3030B_DC(STATUS) PIN_OUT(NV3030B_DC_GPIO, NV3030B_DC_PIN, STATUS)
#define NV3030B_CS(STATUS) PIN_OUT(NV3030B_CS_GPIO, NV3030B_CS_PIN, STATUS)
#define NV3030B_RST(STATUS) PIN_OUT(NV3030B_RST_GPIO, NV3030B_RST_PIN, STATUS)

#define NV3030B_DEFAULT_DISPLAY_DIR      (NV3030B_PORTAIT)                  
#define NV3030B_DEFAULT_PENCOLOR         (RGB565_WHITE)                     
#define NV3030B_DEFAULT_BGCOLOR          (RGB565_BLACK)                          
#define NV3030B_DEFAULT_DISPLAY_FONT     (NV3030B_12X16_OCR)                      

#define CIRCLE_UPPER_RIGHT      0x01
#define CIRCLE_UPPER_LEFT       0x02
#define CIRCLE_LOWER_LEFT       0x04
#define CIRCLE_LOWER_RIGHT      0x08
#define CIRCLE_DRAW_ALL         (CIRCLE_UPPER_RIGHT | CIRCLE_UPPER_LEFT | CIRCLE_LOWER_LEFT | CIRCLE_LOWER_RIGHT)

extern uint16 NV3030B_penColor;
extern uint16 NV3030B_backgroundColor;

typedef enum
{
    NORMAL = 1,
    XOR
} NV3030B_ColorMode_e;

typedef enum
{
    NV3030B_PORTAIT                      = 0,                                    
    NV3030B_PORTAIT_180                  = 1,                                    
    NV3030B_CROSSWISE                    = 2,                                  
    NV3030B_CROSSWISE_180                = 3,                                
}nv3030b_dir_enum;

typedef enum
{
    NV3030B_6X8_FONT                     = 0,                                    // 6x8    
    NV3030B_8X16_FONT                    = 1,                                    // 8x16   
    NV3030B_8X16_OCRB,              
    NV3030B_10X16_OCR,    
    NV3030B_12X16_OCR,    
    NV3030B_12X16_OCRB,   
    NV3030B_12X24_AGENCY,    
    NV3030B_16X24_OCR,    
		NV3030B_16X24_OCRB,      //不好看       
    NV3030B_NUM_FONT          
}Font_Type_t;

void NV3030B_SetFont(Font_Type_t font);
void NV3030B_DrawBMP232(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* pic);
void NV3030B_DrawBMP565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* pic);

void NV3030B_SetRegion (const uint16 x1, const uint16 y1, const uint16 x2, const uint16 y2);
void NV3030B_DMA_Transfer(uint8_t *pData, uint16_t size, DMA_MEMINC_STATE state);
void NV3030B_SetRotation(nv3030b_dir_enum dir);

void NV3030B_SendBuffer(void);
void NV3030B_ClearBuffer(void);

void NV3030B_SetDrawColor(NV3030B_ColorMode_e mode);
void NV3030B_DrawPoint (int16 x, int16 y, uint16 color);
void NV3030B_DrawPoint8(int16 x, int16 y, const uint8 color);
void NV3030B_DrawLine (int16 x_start, int16 y_start, int16 x_end, int16 y_end, uint16 color);
void NV3030B_ShowChar(int16 x, int16 y, char dat);
void NV3030B_ShowStr (int16 x, int16 y, const char dat[]);
void NV3030B_ShowStrMutiRow(int16 x, int16 y, int16 width, const char dat[]);
void NV3030B_ShowInt(int16 x, int16 y, int32 dat, uint8 num);
void NV3030B_ShowUint(int16 x, int16 y, uint32 dat, uint8 num);
void NV3030B_ShowFloat(int16 x, int16 y, float dat, uint8 num, uint8 pointnum);

extern bool reversedColor;
void NV3030B_ModifyColor(void);

void NV3030B_DrawCircle(int16_t x, int16_t y, uint16_t r, uint16_t color, uint8_t section);
void NV3030B_DrawDisc(int16_t x, int16_t y, uint16_t r, uint16_t color, uint8_t section);
void NV3030B_DrawFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);
void NV3030B_DrawBox(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);
void NV3030B_DrawRFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, uint8_t r);
void NV3030B_DrawRBox(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, uint8_t r);
void NV3030B_DrawRBoxWithBlur(int16_t x, int16_t y, uint16_t width, uint16_t height);

void NV3030B_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *pic);
void NV3030B_ShowGrayImage(uint16_t x, uint16_t y, const uint8_t *image, uint16_t width, uint16_t height, uint16_t dis_width,
                          uint16_t dis_height, uint8_t threshold);

void NV3030B_Triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void NV3030B_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void NV3030B_SetColor (const uint16 pen, const uint16 bgcolor);
void NV3030B_Init (uint16 *gram);
void NV3030B_FastHLine(int16_t x, int16_t y, int16_t length, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif
