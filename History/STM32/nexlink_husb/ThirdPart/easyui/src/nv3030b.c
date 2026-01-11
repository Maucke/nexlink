/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#include "nv3030b.h"

#define ST7789 0

uint16                   NV3030B_penColor     = NV3030B_DEFAULT_PENCOLOR;
uint16                   NV3030B_backgroundColor      = NV3030B_DEFAULT_BGCOLOR;

__IO nv3030b_dir_enum          nv3030b_display_dir  = NV3030B_DEFAULT_DISPLAY_DIR;
static Font_Type_t    nv3030b_display_font = NV3030B_DEFAULT_DISPLAY_FONT;
static uint16                    nv3030b_x_max        = LCD_W;
static uint16                    nv3030b_y_max        = LCD_H;
static uint8_t NV3030B_colorMode = NORMAL;
static uint8_t NV3030B_buffer[LCD_H][LCD_W] = {0};
uint16* localgram;

void NV3030B_SetFont(Font_Type_t font)
{
  nv3030b_display_font = font;
}

bool reversedColor = false;
// LCD串行数据写入
static void spi_write_bus(uint8_t dat)
{
  HAL_SPI_Transmit(&hspi1, &dat, 1, 100);
}

// LCD写入8位数据
void spi_write_8bit(uint8_t dat)
{
  NV3030B_CS(0);

  spi_write_bus(dat);

  NV3030B_CS(1);
}

// LCD写入16位数据
void spi_write_16bit(uint16_t dat)
{
  NV3030B_CS(0);

  spi_write_bus(dat >> 8);
  spi_write_bus(dat);

  NV3030B_CS(1);
}

#define nv3030b_write_8bit_data(data)    (spi_write_8bit( (data)))
#define nv3030b_write_16bit_data(data)   (spi_write_16bit( (data)))

static void nv3030b_write_index(const uint8 dat)
{
  NV3030B_DC(0);
  nv3030b_write_8bit_data(dat);
  NV3030B_DC(1);
}


void NV3030B_SetRegion(const uint16 x1, const uint16 y1, const uint16 x2, const uint16 y2)
{
  while(hspi1.State != HAL_SPI_STATE_READY)
    ; // 等待SPI空闲
  NV3030B_CS(1);
  if(nv3030b_display_dir == NV3030B_PORTAIT)
  {
    nv3030b_write_index(0x2a);                                               // 列地址设置
    nv3030b_write_16bit_data(x1);
    nv3030b_write_16bit_data(x2);
    nv3030b_write_index(0x2b);                                               // 行地址设置
    nv3030b_write_16bit_data(y1 + 20);
    nv3030b_write_16bit_data(y2 + 20);
    nv3030b_write_index(0x2c);                                               // 储存器写
  }
  else if(nv3030b_display_dir == NV3030B_PORTAIT_180)
  {
    nv3030b_write_index(0x2a);                                               // 列地址设置
    nv3030b_write_16bit_data(x1);
    nv3030b_write_16bit_data(x2);
    nv3030b_write_index(0x2b);                                               // 行地址设置
    nv3030b_write_16bit_data(y1 + 20);
    nv3030b_write_16bit_data(y2 + 20);
    nv3030b_write_index(0x2c);                                               // 储存器写
  }
  else if(nv3030b_display_dir == NV3030B_CROSSWISE)
  {
    nv3030b_write_index(0x2a);                                               // 列地址设置
    nv3030b_write_16bit_data(x1 + 20);
    nv3030b_write_16bit_data(x2 + 20);
    nv3030b_write_index(0x2b);                                               // 行地址设置
    nv3030b_write_16bit_data(y1);
    nv3030b_write_16bit_data(y2);
    nv3030b_write_index(0x2c);                                               // 储存器写
  }
  else
  {
    nv3030b_write_index(0x2a);                                               // 列地址设置
    nv3030b_write_16bit_data(x1 + 20);
    nv3030b_write_16bit_data(x2 + 20);
    nv3030b_write_index(0x2b);                                               // 行地址设置
    nv3030b_write_16bit_data(y1);
    nv3030b_write_16bit_data(y2);
    nv3030b_write_index(0x2c);                                               // 储存器写
  }
  NV3030B_CS(0);
}

void NV3030B_DMA_Transfer(uint8_t* pData, uint16_t size, DMA_MEMINC_STATE state)
{
  while(hspi1.State != HAL_SPI_STATE_READY)
    ; // 等待SPI空闲
//	// 清除 DMA 控制寄存器的相关设置
////    NV3030B_SPI_TX_DMA->CR &= ~(DMA_SxCR_MINC|SPI_CR1_DFF);
//		NV3030B_SPI_TX_DMA->CR &= ~(DMA_SxCR_MINC);
//    // 根据传入的状态设置是否使能存储器地址增量
//    if (state == DMA_MEMINC_ENABLE)
//        NV3030B_SPI_TX_DMA->CR |= DMA_SxCR_MINC;
////        NV3030B_SPI_TX_DMA->CR |= SPI_CR1_DFF;

  HAL_SPI_Transmit_DMA(&hspi1, pData, size); // 启用DMA传输
}

void NV3030B_SetDrawColor(NV3030B_ColorMode_e mode)
{
  NV3030B_colorMode = mode;
}

void NV3030B_SetRotation(nv3030b_dir_enum dir)
{
  nv3030b_display_dir = dir;
#if ST7789 == 0
  nv3030b_write_index(0x36);
  if(nv3030b_display_dir == NV3030B_PORTAIT)spi_write_8bit(0x08);
  else if(nv3030b_display_dir == NV3030B_PORTAIT_180)spi_write_8bit(0xC8);
  else if(nv3030b_display_dir == NV3030B_CROSSWISE)spi_write_8bit(0x78);
  else spi_write_8bit(0xA8);
#else
  nv3030b_write_index(0x36);
  if(nv3030b_display_dir == NV3030B_PORTAIT)spi_write_8bit(0x00);
  else if(nv3030b_display_dir == NV3030B_PORTAIT_180)spi_write_8bit(0xC0);
  else if(nv3030b_display_dir == NV3030B_CROSSWISE)spi_write_8bit(0x70);
  else spi_write_8bit(0xA0);
#endif
}


uint8_t color16to8(uint16_t c)
{
  return ((c & 0xE000) >> 8) | ((c & 0x0700) >> 6) | ((c & 0x0018) >> 3);
}

__inline uint16_t color8to16(uint8_t color)
{
  uint8_t  blue[] = {0, 11, 21, 31}; // blue 2 to 5 bit colour lookup table
  uint16_t color16 = 0;

  //        =====Green=====     ===============Red==============
  color16  = (color & 0x1C) << 6 | (color & 0xC0) << 5 | (color & 0xE0) << 8;
  //        =====Green=====    =======Blue======
  color16 |= (color & 0x1C) << 3 | blue[color & 0x03];

  return (color16 >> 8) | (color16 << 8);
}

static __IO bool ramindex = 0;
/*!
 * @brief   Send buffer to nv3030b
 *
 * @param   void
 * @return  void
 */
void NV3030B_SendBuffer()
{
  uint8* re_buffer = (uint8*)NV3030B_buffer;
  NV3030B_SetRegion(0, 0, nv3030b_x_max - 1, nv3030b_y_max - 1);

  for(int i = 0; i < (nv3030b_x_max * nv3030b_y_max) / 480; i++)
  {
    ramindex = (ramindex + 1) % 2;
    for(int j = 0; j < 480; j++)
      localgram[j + ramindex * 480] = color8to16(re_buffer[i * 480 + j]);
    NV3030B_DMA_Transfer((uint8_t*)(localgram + ramindex * 480), 480 * 2, DMA_MEMINC_ENABLE);
  }
}

/*!
 * @brief   Clear buffer array
 *
 * @param   void
 * @return  void
 *
 * @note    Use black background color can easily clear the buffer by memset,
 *          but other color cannot and on board flash is not enough for another buffer
 *          to use memcpy, so using color other than black will slower this function.
 */
void NV3030B_ClearBuffer()
{
  uint8_t background = color16to8(NV3030B_backgroundColor);
  if(!reversedColor)
    memset(NV3030B_buffer, background, nv3030b_x_max * nv3030b_y_max * sizeof(uint8_t));
  else
    memset(NV3030B_buffer, ~background, nv3030b_x_max * nv3030b_y_max * sizeof(uint8_t));
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 画点
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     color           颜色格式 RGB565 或者可以使用 zf_common_font.h 内 rgb565_color_enum 枚举值或者自行写入
// 返回参数     void
// 使用示例     nv3030b_draw_point(0, 0, RGB565_RED);            // 坐标 0,0 画一个红色的点
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
__inline void NV3030B_DrawPoint(int16 x, int16 y, const uint16 color)
{
  if(x < nv3030b_x_max && y < nv3030b_y_max && x >= 0 && y >= 0)
  {
    if(NV3030B_colorMode == XOR)
    {
      if(NV3030B_buffer[y][x] == color16to8(color))
        NV3030B_buffer[y][x] = NV3030B_backgroundColor;
      else
        NV3030B_buffer[y][x] ^= color16to8(color);
    }
    else
    {
      if(!reversedColor)
        NV3030B_buffer[y][x] = color16to8(color);
      else
        NV3030B_buffer[y][x] = ~color16to8(color);
    }
  }
}
__inline void NV3030B_DrawPoint8(int16 x, int16 y, const uint8 color)
{
  if(x < nv3030b_x_max && y < nv3030b_y_max && x >= 0 && y >= 0)
  {
    if(NV3030B_colorMode == XOR)
    {
      if(NV3030B_buffer[y][x] == (color))
        NV3030B_buffer[y][x] = NV3030B_backgroundColor;
      else
        NV3030B_buffer[y][x] ^= (color);
    }
    else
    {
      if(!reversedColor)
        NV3030B_buffer[y][x] = (color);
      else
        NV3030B_buffer[y][x] = ~(color);
    }
  }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 画线
// 参数说明     x_start         坐标x方向的起点
// 参数说明     y_start         坐标y方向的起点
// 参数说明     x_end           坐标x方向的终点
// 参数说明     y_end           坐标y方向的终点
// 参数说明     color           颜色格式 RGB565 或者可以使用 zf_common_font.h 内 rgb565_color_enum 枚举值或者自行写入
// 返回参数     void
// 使用示例     nv3030b_draw_line(0, 0, 10, 10, RGB565_RED);     // 坐标 0,0 到 10,10 画一条红色的线
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_DrawLine(int16 x_start, int16 y_start, int16 x_end, int16 y_end, const uint16 color)
{
  int16 x_dir = (x_start < x_end ? 1 : -1);
  int16 y_dir = (y_start < y_end ? 1 : -1);
  float temp_rate = 0;
  float temp_b = 0;

  do
  {
    if(x_start != x_end)
    {
      temp_rate = (float)(y_start - y_end) / (float)(x_start - x_end);
      temp_b = (float) y_start - (float) x_start * temp_rate;
    }
    else
    {
      while(y_start != y_end)
      {
        NV3030B_DrawPoint(x_start, y_start, color);
        y_start += y_dir;
      }
      break;
    }
    if(func_abs(y_start - y_end) > func_abs(x_start - x_end))
    {
      while(y_start != y_end)
      {
        NV3030B_DrawPoint(x_start, y_start, color);
        y_start += y_dir;
        x_start = (int16)(((float) y_start - temp_b) / temp_rate);
      }
    }
    else
    {
      while(x_start != x_end)
      {
        NV3030B_DrawPoint(x_start, y_start, color);
        x_start += x_dir;
        y_start = (int16)((float) x_start * temp_rate + temp_b);
      }
    }
  }
  while(0);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示字符
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     dat             需要显示的字符
// 返回参数     void
// 使用示例     nv3030b_show_char(0, 0, 'x');                    // 坐标 0,0 写一个字符 x
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_ShowChar(int16 x, int16 y, const char dat)
{
  uint8 i, j;
  switch(nv3030b_display_font)
  {
  case NV3030B_6X8_FONT:
    for(i = 0; i < 6; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_default_6x8[dat - 32][i];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
    }
    break;
  case NV3030B_8X16_FONT:
    for(i = 0; i < 8; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_default_8x16[dat - 32][i];
      uint8 temp_bottom = ascii_default_8x16[dat - 32][i + 8];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;

  case NV3030B_8X16_OCRB:
    for(i = 0; i < 8; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCRB_8x16[dat - 32][i];
      uint8 temp_bottom = ascii_OCRB_8x16[dat - 32][i + 8];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_10X16_OCR:
    for(i = 0; i < 10; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCR_10x16[dat - 32][i];
      uint8 temp_bottom = ascii_OCR_10x16[dat - 32][i + 10];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_12X16_OCR:
    for(i = 0; i < 12; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCR_12x16[dat - 32][i];
      uint8 temp_bottom = ascii_OCR_12x16[dat - 32][i + 12];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_12X16_OCRB:
    for(i = 0; i < 12; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCRB_12x16[dat - 32][i];
      uint8 temp_bottom = ascii_OCRB_12x16[dat - 32][i + 12];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_12X24_AGENCY:
    for(i = 0; i < 12; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_agency_12x24[dat - 32][i];
      uint8 temp_mid = ascii_agency_12x24[dat - 32][i + 12];
      uint8 temp_bottom = ascii_agency_12x24[dat - 32][i + 24];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_mid & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_mid >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 16, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_16X24_OCR:
    for(i = 0; i < 16; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCR_16x24[dat - 32][i];
      uint8 temp_mid = ascii_OCR_16x24[dat - 32][i + 16];
      uint8 temp_bottom = ascii_OCR_16x24[dat - 32][i + 32];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_mid & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_mid >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 16, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;
  case NV3030B_16X24_OCRB:
    for(i = 0; i < 16; i++)
    {
      // 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
      uint8 temp_top = ascii_OCRB_16x24[dat - 32][i];
      uint8 temp_mid = ascii_OCRB_16x24[dat - 32][i + 16];
      uint8 temp_bottom = ascii_OCRB_16x24[dat - 32][i + 32];
      for(j = 0; j < 8; j++)
      {
        if(temp_top & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j, NV3030B_penColor);
        }
        temp_top >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_mid & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 8, NV3030B_penColor);
        }
        temp_mid >>= 1;
      }
      for(j = 0; j < 8; j++)
      {
        if(temp_bottom & 0x01)
        {
          NV3030B_DrawPoint(x + i, y + j + 16, NV3030B_penColor);
        }
        temp_bottom >>= 1;
      }
    }
    break;

  default:
    break;
  }
}

void NV3030B_ShowStrMutiRow(int16 x, int16 y, int16 width, const char dat[])
{
  uint16 j = 0;
  int16 x_start = x;
//  int16 y_start = y;
  while(dat[j] != '\0')
  {
    switch(nv3030b_display_font)
    {
    case NV3030B_6X8_FONT:
      if(dat[j] == '\n')
      {
        x = x_start;
        y += 8 + 2;
				while(dat[j] == ' ')
				{
					j++;
				}
      }
      else
      {
        if(x >= x_start + width - 6)
        {
          x = x_start;
          y += 8 + 2;
					while(dat[j] == ' ')
					{
						j++;
					}
        }
        NV3030B_ShowChar(x, y, dat[j]);
        x += 6;
      }
      break;
    default:
      break;
		}
    if(y >= LCD_H)return;
    j++;
	}
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示字符串
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     dat             需要显示的字符串
// 返回参数     void
// 使用示例     nv3030b_show_string(0, 0, "seekfree");
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_ShowStr(int16 x, int16 y, const char dat[])
{
  uint16 j = 0;
  int16 x_start = x;
//  int16 y_start = y;
  while(dat[j] != '\0')
  {
    switch(nv3030b_display_font)
    {
    case NV3030B_6X8_FONT:
      if(dat[j] == '\n')
      {
        x = x_start;
        y += 8 + 2;
      }
			else
      {
        if(x > LCD_W - 6*8)
        {
					if(dat[j] != '\0'&&dat[j+1] != '\0'&&dat[j+2] != '\0')
					{
						for(int i=0;i<3;i++)
						{
							NV3030B_ShowChar(x, y, '.');
							x+=6;
						}
						return;
					}
        }
        NV3030B_ShowChar(x, y, dat[j]);
        x += 6;
      }
      break;
    case NV3030B_8X16_FONT:
    case NV3030B_8X16_OCRB:
      NV3030B_ShowChar(x, y, dat[j]);
      x += 8;
      if(x >= LCD_W - 8)
      {
        x = x_start;
        y += 16 + 4;
      }
      break;
    case NV3030B_10X16_OCR:
      NV3030B_ShowChar(x, y, dat[j]);
      x += 10;
      if(x >= LCD_W - 10)
      {
        x = x_start;
        y += 16 + 4;
      }
      break;
    case NV3030B_12X16_OCR:
    case NV3030B_12X16_OCRB:
      NV3030B_ShowChar(x, y, dat[j]);
      x += 12;
      if(x >= LCD_W - 12)
      {
        x = x_start;
        y += 16 + 24;
      }
      break;
    case NV3030B_12X24_AGENCY:
      NV3030B_ShowChar(x, y, dat[j]);
      x += 12;
      if(x >= LCD_W - 12)
      {
        x = x_start;
        y += 24 + 6;
      }
      break;
    case NV3030B_16X24_OCR:
    case NV3030B_16X24_OCRB:
      NV3030B_ShowChar(x, y, dat[j]);
      x += 16;
      if(x >= LCD_W - 16)
      {
        x = x_start;
        y += 24 + 6;
      }
      break;
    default:
      break;
    }
    if(y >= LCD_H)return;
    j++;
  }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示32位有符号 (去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     dat             需要显示的变量 数据类型 int32
// 参数说明     num             需要显示的位数 最高10位  不包含正负号
// 返回参数     void
// 使用示例     nv3030b_show_int(0, 0, x, 3);                    // x 可以为 int32 int16 int8 类型
// 备注信息     负数会显示一个 ‘-’号   正数显示一个空格
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_ShowInt(int16 x, int16 y, const int32 dat, uint8 num)
{
  assert_param(num > 0);
  assert_param(num <= 10);

  int32 dat_temp = dat;
  int32 offset = 1;
  char data_buffer[12];

  memset(data_buffer, 0, 12);
  memset(data_buffer, ' ', num + 1);

  if(num < 10)
  {
    for(; num > 0; num--)
    {
      offset *= 10;
    }
    dat_temp %= offset;
  }
  func_int_to_str(data_buffer, dat_temp);
  NV3030B_ShowStr(x, y, (const char*) &data_buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示32位无符号 (去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     dat             需要显示的变量 数据类型 uint32
// 参数说明     num             需要显示的位数 最高10位  不包含正负号
// 返回参数     void
// 使用示例     nv3030b_show_uint(0, 0, x, 3);                   // x 可以为 uint32 uint16 uint8 类型
// 备注信息     负数会显示一个 ‘-’号   正数显示一个空格
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_ShowUint(int16 x, int16 y, const uint32 dat, uint8 num)
{
  assert_param(num > 0);
  assert_param(num <= 10);

  uint32 dat_temp = dat;
  int32 offset = 1;
  char data_buffer[12];
  memset(data_buffer, 0, 12);
  memset(data_buffer, ' ', num);

  if(num < 10)
  {
    for(; num > 0; num--)
    {
      offset *= 10;
    }
    dat_temp %= offset;
  }
  func_uint_to_str(data_buffer, dat_temp);
  NV3030B_ShowStr(x, y, (const char*) &data_buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示浮点数 (去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     dat             需要显示的变量 数据类型 float 或 double
// 参数说明     num             整数位显示长度   最高8位
// 参数说明     pointnum        小数位显示长度   最高6位
// 返回参数     void
// 使用示例     nv3030b_show_float(0, 0, x, 2, 3);               // 显示浮点数 整数显示 2 位 小数显示 3 位
// 备注信息     特别注意当发现小数部分显示的值与你写入的值不一样的时候，
//              可能是由于浮点数精度丢失问题导致的，这并不是显示函数的问题，
//              有关问题的详情，请自行百度学习   浮点数精度丢失问题。
//              负数会显示一个 ‘-’号   正数显示一个空格
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_ShowFloat(int16 x, int16 y, const float dat, uint8 num, uint8 pointnum)
{
  assert_param(num > 0);
  assert_param(num <= 8);
  assert_param(pointnum > 0);
  assert_param(pointnum <= 6);

  float dat_temp = dat;
  float offset = 1.0;
  char data_buffer[17];
  memset(data_buffer, 0, 17);
  memset(data_buffer, ' ', num + pointnum + 2);

  if(num < 10)
  {
    for(; num > 0; num--)
    {
      offset *= 10;
    }
    dat_temp = dat_temp - ((int) dat_temp / (int) offset) * offset;
  }
  func_float_to_str(data_buffer, dat_temp, pointnum);
  NV3030B_ShowStr(x, y, data_buffer);
}


void NV3030B_ModifyColor()
{
//    if (reversedColor)
//    {
//        NV3030B_penColor = NV3030B_DEFAULT_BGCOLOR;
//        NV3030B_backgroundColor = NV3030B_DEFAULT_PENCOLOR;
//    } else
//    {
//        NV3030B_penColor = NV3030B_DEFAULT_PENCOLOR;
//        NV3030B_backgroundColor = NV3030B_DEFAULT_BGCOLOR;
//    }
}


/*!
 * @brief   Draw circle / disc with 5 choices of section
 *
 * @param   x           Center x of the circle
 * @param   y           Center y of the circle
 * @param   r           Radius
 * @param   color       Color
 * @param   section     See definition in user_nv3030b.h
 * @return  void
 */
void NV3030B_DrawCircle(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section)
{
  // y = kx, k = 1
  uint16_t x0 = (uint16_t)(r * cos(0.01745 * 45));
  uint16_t fx;

  // x^2 + y^2 = r^2, y->x / x->y
  for(int i = -x0 + 1; i < 0; ++i)
  {
    fx = (uint16_t) sqrt(pow(r, 2) - pow(i, 2));
    if(section & CIRCLE_UPPER_RIGHT)
    {
      NV3030B_DrawPoint(x - i, y - fx, color);
      NV3030B_DrawPoint(x + fx, y + i, color);
    }
    if(section & CIRCLE_UPPER_LEFT)
    {
      NV3030B_DrawPoint(x + i, y - fx, color);
      NV3030B_DrawPoint(x - fx, y + i, color);
    }
    if(section & CIRCLE_LOWER_LEFT)
    {
      NV3030B_DrawPoint(x + i, y + fx, color);
      NV3030B_DrawPoint(x - fx, y - i, color);
    }
    if(section & CIRCLE_LOWER_RIGHT)
    {
      NV3030B_DrawPoint(x - i, y + fx, color);
      NV3030B_DrawPoint(x + fx, y - i, color);
    }
  }

  // Add support for XOR color mode
  fx = (uint16_t) sqrt(pow(r, 2) - pow(x0, 2));
  if(section & CIRCLE_UPPER_RIGHT)
  {
    if(r > 1)
    {
      NV3030B_DrawPoint(x + r, y, color);
      NV3030B_DrawPoint(x, y - r, color);
    }
    if(x0 == fx)
      NV3030B_DrawPoint(x + x0, y - x0, color);
    else
    {
      NV3030B_DrawPoint(x + x0, y - fx, color);
      NV3030B_DrawPoint(x + fx, y - x0, color);
    }
  }
  if(section & CIRCLE_UPPER_LEFT)
  {
    if(r > 1)
    {
      NV3030B_DrawPoint(x - r, y, color);
      NV3030B_DrawPoint(x, y - r, color);
    }
    if(x0 == fx)
      NV3030B_DrawPoint(x - x0, y - x0, color);
    else
    {
      NV3030B_DrawPoint(x - x0, y - fx, color);
      NV3030B_DrawPoint(x - fx, y - x0, color);
    }
  }
  if(section & CIRCLE_LOWER_LEFT)
  {
    if(r > 1)
    {
      NV3030B_DrawPoint(x - r, y, color);
      NV3030B_DrawPoint(x, y + r, color);
    }
    if(x0 == fx)
      NV3030B_DrawPoint(x - x0, y + x0, color);
    else
    {
      NV3030B_DrawPoint(x - x0, y + fx, color);
      NV3030B_DrawPoint(x - fx, y + x0, color);
    }
  }
  if(section & CIRCLE_LOWER_RIGHT)
  {
    if(r > 1)
    {
      NV3030B_DrawPoint(x + r, y, color);
      NV3030B_DrawPoint(x, y + r, color);
    }
    if(x0 == fx)
      NV3030B_DrawPoint(x + x0, y + x0, color);
    else
    {
      NV3030B_DrawPoint(x + x0, y + fx, color);
      NV3030B_DrawPoint(x + fx, y + x0, color);
    }
  }
  if(section == CIRCLE_DRAW_ALL)
  {
    NV3030B_DrawPoint(x + r, y, color);
    NV3030B_DrawPoint(x - r, y, color);
    NV3030B_DrawPoint(x, y - r, color);
    NV3030B_DrawPoint(x, y + r, color);
  }
}
void NV3030B_DrawDisc(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section)
{
  // y = kx, k = 1
  uint16_t x0 = (uint16_t)(r * cos(0.01745 * 45));
  uint16_t fx;

  // x^2 + y^2 = r^2, y->x / x->y
  for(int i = -x0 + 1; i < 0; ++i)
  {
    fx = (uint16_t) sqrt(pow(r, 2) - pow(i, 2));
    if(section & CIRCLE_UPPER_RIGHT)
    {
      NV3030B_DrawLine(x - i, y - fx, x - i, y + i, color);
      NV3030B_DrawLine(x + fx, y + i, x - i, y + i, color);
    }
    if(section & CIRCLE_UPPER_LEFT)
    {
      NV3030B_DrawLine(x + i, y - fx, x + i, y + i, color);
      NV3030B_DrawLine(x - fx, y + i, x + i, y + i, color);
    }
    if(section & CIRCLE_LOWER_LEFT)
    {
      NV3030B_DrawLine(x + i, y + fx, x + i, y - i, color);
      NV3030B_DrawLine(x - fx, y - i, x + i, y - i, color);
    }
    if(section & CIRCLE_LOWER_RIGHT)
    {
      NV3030B_DrawLine(x - i, y + fx, x - i, y - i, color);
      NV3030B_DrawLine(x + fx, y - i, x - i, y - i, color);
    }
  }

  // Add support for XOR color mode
  NV3030B_DrawPoint(x, y, color);
  if(r != 2)
    NV3030B_DrawPoint(x, y, color);

  fx = (uint16_t) sqrt(pow(r, 2) - pow(x0, 2));
  if(section & CIRCLE_UPPER_RIGHT)
  {
    if(r > 1)
    {
      NV3030B_DrawLine(x + r, y, x, y, color);
      NV3030B_DrawLine(x, y - r, x, y, color);
    }
    if(r > 2)
      NV3030B_DrawLine(x, y, x + x0, y - x0, color);
    if(x0 == fx)
      NV3030B_DrawPoint(x + x0, y - x0, color);
    else
    {
      NV3030B_DrawPoint(x + x0, y - fx, color);
      NV3030B_DrawPoint(x + fx, y - x0, color);
      NV3030B_DrawPoint(x + x0, y - x0, color);
    }
  }
  if(section & CIRCLE_UPPER_LEFT)
  {
    if(r > 1)
    {
      NV3030B_DrawLine(x - r, y, x, y, color);
      NV3030B_DrawLine(x, y - r, x, y, color);
    }
    if(r > 2)
      NV3030B_DrawLine(x, y, x - x0, y - x0, color);
    if(x0 == fx)
      NV3030B_DrawPoint(x - x0, y - x0, color);
    else
    {
      NV3030B_DrawPoint(x - x0, y - fx, color);
      NV3030B_DrawPoint(x - fx, y - x0, color);
      NV3030B_DrawPoint(x - x0, y - x0, color);
    }
  }
  if(section & CIRCLE_LOWER_LEFT)
  {
    if(r > 1)
    {
      NV3030B_DrawLine(x - r, y, x, y, color);
      NV3030B_DrawLine(x, y + r, x, y, color);
    }
    if(r > 2)
      NV3030B_DrawLine(x, y, x - x0, y + x0, color);
    if(x0 == fx)
      NV3030B_DrawPoint(x - x0, y + x0, color);
    else
    {
      NV3030B_DrawPoint(x - x0, y + fx, color);
      NV3030B_DrawPoint(x - fx, y + x0, color);
      NV3030B_DrawPoint(x - x0, y + x0, color);
    }
  }
  if(section & CIRCLE_LOWER_RIGHT)
  {
    if(r > 1)
    {
      NV3030B_DrawLine(x + r, y, x, y, color);
      NV3030B_DrawLine(x, y + r, x, y, color);
    }
    if(r > 2)
      NV3030B_DrawLine(x, y, x + x0, y + x0, color);
    if(x0 == fx)
      NV3030B_DrawPoint(x + x0, y + x0, color);
    else
    {
      NV3030B_DrawPoint(x + x0, y + fx, color);
      NV3030B_DrawPoint(x + fx, y + x0, color);
      NV3030B_DrawPoint(x + x0, y + x0, color);
    }
  }
  if(section == CIRCLE_DRAW_ALL)
  {
    NV3030B_DrawLine(x + r, y, x, y, color);
    NV3030B_DrawLine(x - r, y, x, y, color);
    NV3030B_DrawLine(x, y - r, x, y, color);
    NV3030B_DrawLine(x, y + r, x, y, color);
    NV3030B_DrawPoint(x, y, color);
  }
}


/*!
 * @brief   Draw box / filled box
 *
 * @param   x       Starting x
 * @param   y       Starting y
 * @param   width   Box width
 * @param   height  Box height
 * @param   color   Color
 * @return  void
 */
void NV3030B_DrawFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color)
{
  for(int i = x; i < x + width; i++)
  {
    NV3030B_DrawPoint(i, y, color);
    NV3030B_DrawPoint(i, y + height - 1, color);
  }
  for(int j = y; j < y + height; j++)
  {
    NV3030B_DrawPoint(x, j, color);
    NV3030B_DrawPoint(x + width - 1, j, color);
  }
}
void NV3030B_DrawBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color)
{
  for(int j = y; j < y + height; ++j)
  {
    for(int i = x; i < x + width; ++i)
    {
//					dbmsg("X:%d,Y:%d",i, j);
      NV3030B_DrawPoint(i, j, color);
    }
  }
}


/*!
 * @brief   Draw rounded box / filled box
 *
 * @param   x       Starting x
 * @param   y       Starting y
 * @param   width   Box width
 * @param   height  Box height
 * @param   color   Color
 * @param   r       Radius
 * @return  void
 */
void NV3030B_DrawRFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r)
{
  for(int i = x + r + 1; i < x + width - r - 1; i++)
  {
    NV3030B_DrawPoint(i, y, color);
    NV3030B_DrawPoint(i, y + height - 1, color);
  }
  for(int j = y + r + 1; j < y + height - r - 1; j++)
  {
    NV3030B_DrawPoint(x, j, color);
    NV3030B_DrawPoint(x + width - 1, j, color);
  }

  NV3030B_DrawCircle(x + r, y + r, r, color, CIRCLE_UPPER_LEFT);
  NV3030B_DrawCircle(x + width - 1 - r, y + r, r, color, CIRCLE_UPPER_RIGHT);
  NV3030B_DrawCircle(x + r, y + height - 1 - r, r, color, CIRCLE_LOWER_LEFT);
  NV3030B_DrawCircle(x + width - 1 - r, y + height - 1 - r, r, color, CIRCLE_LOWER_RIGHT);
}
void NV3030B_DrawRBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r)
{
  NV3030B_DrawDisc(x + r, y + r, r, color, CIRCLE_UPPER_LEFT);
  NV3030B_DrawDisc(x + width - 1 - r, y + r, r, color, CIRCLE_UPPER_RIGHT);
  NV3030B_DrawDisc(x + r, y + height - 1 - r, r, color, CIRCLE_LOWER_LEFT);
  NV3030B_DrawDisc(x + width - 1 - r, y + height - 1 - r, r, color, CIRCLE_LOWER_RIGHT);

  NV3030B_DrawBox(x + r + 1, y, width - 2 - 2 * r, r + 1, color);
  NV3030B_DrawBox(x, y + r + 1, width, height - 2 * r - 2, color);
  NV3030B_DrawBox(x + r + 1, y + height - 1 - r, width - 2 - 2 * r, r + 1, color);
}


/*!
 * @brief   Show Binary BMP photo on screen
 *
 * @param   x       Starting x
 * @param   y       Starting y
 * @param   width   Pic width
 * @param   height  Pic height
 * @param   pic     The array of picture(阴码 逐行式 逆向)
 */
void NV3030B_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* pic)
{
  uint8_t temp, j;
  uint8_t x0 = x;
  uint8_t* tmp = (uint8_t*) pic;
  uint16_t i, picSize = 0;

  picSize = (width / 8 + ((width % 8) ? 1 : 0)) * height;

  for(i = 0; i < picSize; i++)
  {
    temp = tmp[i];
    for(j = 0; j < 8; j++)
    {
      if(temp & 0x01)
      {
        NV3030B_DrawPoint(x, y, NV3030B_penColor);
      }
      temp >>= 1;
      x++;

      if((x - x0) == width)
      {
        x = x0;
        y++;
        break;
      }
    }
  }
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     NV3030B 显示 8bit 灰度图像 带二值化阈值
// 参数说明     x               坐标x方向的起点 参数范围 [0, nv3030b_x_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, nv3030b_y_max-1]
// 参数说明     *image          图像数组指针
// 参数说明     width           图像实际宽度
// 参数说明     height          图像实际高度
// 参数说明     dis_width       图像显示宽度 参数范围 [0, nv3030b_x_max]
// 参数说明     dis_height      图像显示高度 参数范围 [0, nv3030b_y_max]
// 参数说明     threshold       二值化显示阈值 0-不开启二值化
// 返回参数     void
// 使用示例     nv3030b_show_gray_image(0, 0, mt9v03x_image[0], MT9V03X_W, MT9V03X_H, MT9V03X_W, MT9V03X_H, 0);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void
NV3030B_ShowGrayImage(uint16_t x, uint16_t y, const uint8_t* image, uint16_t width, uint16_t height, uint16_t dis_width,
                      uint16_t dis_height, uint8_t threshold)
{
  uint16_t i, j;
  uint16_t color, temp;
  uint32_t width_index, height_index;

  NV3030B_CS(0);
  for(j = 0; j < dis_height; j++)
  {
    height_index = j * height / dis_height;
    for(i = 0; i < dis_width; i++)
    {
      width_index = i * width / dis_width;
      temp = *(image + height_index * width + width_index);
      if(threshold == 0)
      {
        color = (0x001f & ((temp) >> 3)) << 11;
        color = color | (((0x003f) & ((temp) >> 2)) << 5);
        color = color | (0x001f & ((temp) >> 3));
        NV3030B_DrawPoint(x + i, y + j, color);
      }
      else if(temp < threshold)
        NV3030B_DrawPoint(x + i, y + j, RGB565_BLACK);
      else
        NV3030B_DrawPoint(x + i, y + j, RGB565_WHITE);
    }
  }
  NV3030B_CS(1);
}
void
NV3030B_Triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  NV3030B_DrawLine(x0, y0, x1, y1, color);
  NV3030B_DrawLine(x1, y1, x2, y2, color);
  NV3030B_DrawLine(x2, y2, x0, y0, color);
}

void NV3030B_FastHLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
  // Bounds check
  int16_t x0 = x;
  do
  {
    NV3030B_DrawPoint(x, y, color);   // 逐点显示，描出垂直线
    x++;
  }
  while(x0 + length >= x);
}
#define swap(a, b) { uint16_t t = a; a = b; b = t; }
void
NV3030B_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{

  int16_t a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if(y0 > y1)
  {
    swap(y0, y1);
    swap(x0, x1);
  }
  if(y1 > y2)
  {
    swap(y2, y1);
    swap(x2, x1);
  }
  if(y0 > y1)
  {
    swap(y0, y1);
    swap(x0, x1);
  }

  if(y0 == y2)
  {
    // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)
    {
      a = x1;
    }
    else if(x1 > b)
    {
      b = x1;
    }
    if(x2 < a)
    {
      a = x2;
    }
    else if(x2 > b)
    {
      b = x2;
    }
    NV3030B_FastHLine(a, y0, b - a, color);
    return;
  }

  int16_t dx01 = x1 - x0,
          dy01 = y1 - y0,
          dx02 = x2 - x0,
          dy02 = y2 - y0,
          dx12 = x2 - x1,
          dy12 = y2 - y1,
          sa = 0,
          sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2)
  {
    last = y1;   // Include y1 scanline
  }
  else
  {
    last = y1 - 1; // Skip it
  }


  for(y = y0; y <= last; y++)
  {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if(a > b)
    {
      swap(a, b);
    }

    NV3030B_FastHLine(a, y, b - a, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);

  for(; y <= y2; y++)
  {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if(a > b)
    {
      swap(a, b);
    }

    NV3030B_FastHLine(a, y, b - a, color);
  }
}

void NV3030B_SetDirection(nv3030b_dir_enum dir)
{
  nv3030b_display_dir = dir;
  switch(nv3030b_display_dir)
  {
  case NV3030B_PORTAIT:
  case NV3030B_PORTAIT_180:
  {
    nv3030b_x_max = nv3030b_y_max;
    nv3030b_y_max = nv3030b_x_max;
  }
  break;
  case NV3030B_CROSSWISE:
  case NV3030B_CROSSWISE_180:
  {
    nv3030b_x_max = nv3030b_x_max;
    nv3030b_y_max = nv3030b_y_max;
  }
  break;
  }
}

void NV3030B_DrawBMP232(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* pic)
{
  for(int j = y; j < y + height; ++j)
  {
    for(int i = x; i < x + width; ++i)
    {
      NV3030B_DrawPoint8(i, j, pic[(j - y)*width + (i - x)]);
    }
  }
}

void NV3030B_DrawBMP565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* pic)
{
  for(int j = y; j < y + height; ++j)
  {
    for(int i = x; i < x + width; ++i)
    {
//					dbmsg("X:%d,Y:%d",i, j);
      NV3030B_DrawPoint(i, j, (pic[(j - y)*width * 2 + 2 * (i - x) + 1]) | (pic[(j - y)*width * 2 + 2 * (i - x)] << 8));
    }
  }
}

void NV3030B_SetColor(const uint16 pen, const uint16 bgcolor)
{
  NV3030B_penColor = pen;
  NV3030B_backgroundColor = bgcolor;
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     1.14寸 IPS液晶初始化
// 参数说明     void
// 返回参数     void
// 使用示例     nv3030b_init();
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void NV3030B_Init(uint16* gram)
{
  localgram = gram;
  NV3030B_RST(0);
  HAL_Delay(200);
  NV3030B_RST(1);
  HAL_Delay(10);
#if ST7789 == 0
  nv3030b_write_index(0xfd);//private_access
  spi_write_8bit(0x06);
  spi_write_8bit(0x08);

  nv3030b_write_index(0x61);//add
  spi_write_8bit(0x07);//
  spi_write_8bit(0x04);//

  nv3030b_write_index(0x62);//bias setting
  spi_write_8bit(0x00);//00
  spi_write_8bit(0x44);//44
  spi_write_8bit(0x45);//40  47

  nv3030b_write_index(0x63);//
  spi_write_8bit(0x41);//
  spi_write_8bit(0x07);//
  spi_write_8bit(0x12);//
  spi_write_8bit(0x12);//

  nv3030b_write_index(0x64);//
  spi_write_8bit(0x37);//
  //VSP
  nv3030b_write_index(0x65);//Pump1=4.7MHz //PUMP1 VSP
  spi_write_8bit(0x09);//D6-5:pump1_clk[1:0] clamp 28 2b
  spi_write_8bit(0x10);//6.26
  spi_write_8bit(0x21);
  //VSN
  nv3030b_write_index(0x66); //pump=2 AVCL
  spi_write_8bit(0x09); //clamp 08 0b 09
  spi_write_8bit(0x10); //10
  spi_write_8bit(0x21);
  //add source_neg_time
  nv3030b_write_index(0x67);//pump_sel
  spi_write_8bit(0x20);//21 20
  spi_write_8bit(0x40);

  //gamma vap/van
  nv3030b_write_index(0x68);//gamma vap/van
  spi_write_8bit(0x90);//
  spi_write_8bit(0x4c);//
  spi_write_8bit(0x7C);//VCOM
  spi_write_8bit(0x66);//

  nv3030b_write_index(0xb1);//frame rate
  spi_write_8bit(0x0F);//0x0f fr_h[5:0] 0F
  spi_write_8bit(0x02);//0x02 fr_v[4:0] 02
  spi_write_8bit(0x01);//0x04 fr_div[2:0] 04

  nv3030b_write_index(0xB4);
  spi_write_8bit(0x01); //01:1dot 00:column
  ////porch
  nv3030b_write_index(0xB5);
  spi_write_8bit(0x02);//0x02 vfp[6:0]
  spi_write_8bit(0x02);//0x02 vbp[6:0]
  spi_write_8bit(0x0a);//0x0A hfp[6:0]
  spi_write_8bit(0x14);//0x14 hbp[6:0]

  nv3030b_write_index(0xB6);
  spi_write_8bit(0x04);//
  spi_write_8bit(0x01);//
  spi_write_8bit(0x9f);//
  spi_write_8bit(0x00);//
  spi_write_8bit(0x02);//
  ////gamme sel
  nv3030b_write_index(0xdf);//
  spi_write_8bit(0x11);//gofc_gamma_en_sel=1
  ////gamma_test1 A1#_wangly
  //3030b_gamma_new_
  //GAMMA---------------------------------/////////////

  //GAMMA---------------------------------/////////////
  nv3030b_write_index(0xE2);
  spi_write_8bit(0x13);//vrp0[5:0]	V0 13
  spi_write_8bit(0x00);//vrp1[5:0]	V1
  spi_write_8bit(0x00);//vrp2[5:0]	V2
  spi_write_8bit(0x30);//vrp3[5:0]	V61
  spi_write_8bit(0x33);//vrp4[5:0]	V62
  spi_write_8bit(0x3f);//vrp5[5:0]	V63

  nv3030b_write_index(0xE5);
  spi_write_8bit(0x3f);//vrn0[5:0]	V63
  spi_write_8bit(0x33);//vrn1[5:0]	V62
  spi_write_8bit(0x30);//vrn2[5:0]	V61
  spi_write_8bit(0x00);//vrn3[5:0]	V2
  spi_write_8bit(0x00);//vrn4[5:0]	V1
  spi_write_8bit(0x13);//vrn5[5:0]  V0 13

  nv3030b_write_index(0xE1);
  spi_write_8bit(0x00);//prp0[6:0]	V15
  spi_write_8bit(0x57);//prp1[6:0]	V51

  nv3030b_write_index(0xE4);
  spi_write_8bit(0x58);//prn0[6:0]	V51
  spi_write_8bit(0x00);//prn1[6:0]  V15

  nv3030b_write_index(0xE0);
  spi_write_8bit(0x01);//pkp0[4:0]	V3
  spi_write_8bit(0x03);//pkp1[4:0]	V7
  spi_write_8bit(0x0d);//pkp2[4:0]	V21
  spi_write_8bit(0x0e);//pkp3[4:0]	V29
  spi_write_8bit(0x0e);//pkp4[4:0]	V37
  spi_write_8bit(0x0c);//pkp5[4:0]	V45
  spi_write_8bit(0x15);//pkp6[4:0]	V56
  spi_write_8bit(0x19);//pkp7[4:0]	V60

  nv3030b_write_index(0xE3);
  spi_write_8bit(0x1a);//pkn0[4:0]	V60
  spi_write_8bit(0x16);//pkn1[4:0]	V56
  spi_write_8bit(0x0C);//pkn2[4:0]	V45
  spi_write_8bit(0x0f);//pkn3[4:0]	V37
  spi_write_8bit(0x0e);//pkn4[4:0]	V29
  spi_write_8bit(0x0d);//pkn5[4:0]	V21
  spi_write_8bit(0x02);//pkn6[4:0]	V7
  spi_write_8bit(0x01);//pkn7[4:0]	V3
  //GAMMA---------------------------------/////////////

  //source
  nv3030b_write_index(0xE6);
  spi_write_8bit(0x00);
  spi_write_8bit(0xff);//SC_EN_START[7:0] f0

  nv3030b_write_index(0xE7);
  spi_write_8bit(0x01);//CS_START[3:0] 01
  spi_write_8bit(0x04);//scdt_inv_sel cs_vp_en
  spi_write_8bit(0x03);//CS1_WIDTH[7:0] 12
  spi_write_8bit(0x03);//CS2_WIDTH[7:0] 12
  spi_write_8bit(0x00);//PREC_START[7:0] 06
  spi_write_8bit(0x12);//PREC_WIDTH[7:0] 12

  nv3030b_write_index(0xE8); //source
  spi_write_8bit(0x00); //VCMP_OUT_EN 81-
  spi_write_8bit(0x70); //chopper_sel[6:4]
  spi_write_8bit(0x00); //gchopper_sel[6:4] 60
  ////gate
  nv3030b_write_index(0xEc);
  spi_write_8bit(0x52);//52

  nv3030b_write_index(0xF1);
  spi_write_8bit(0x01);//te_pol tem_extend 00 01 03
  spi_write_8bit(0x01);
  spi_write_8bit(0x02);


  nv3030b_write_index(0xF6);
  spi_write_8bit(0x09);
  spi_write_8bit(0x10);
  spi_write_8bit(0x00);//
  spi_write_8bit(0x00);//40 3线2通道

  nv3030b_write_index(0xfd);
  spi_write_8bit(0xfa);
  spi_write_8bit(0xfc);

  nv3030b_write_index(0x3a);
  spi_write_8bit(0x05);//

  nv3030b_write_index(0x35);
  spi_write_8bit(0x00);

  NV3030B_SetRotation(nv3030b_display_dir);

  nv3030b_write_index(0x21);

  nv3030b_write_index(0x11); // exit sleep
  HAL_Delay(200);
  nv3030b_write_index(0x29); // display on
  HAL_Delay(10);
#else
  //************* Start Initial Sequence **********//
  nv3030b_write_index(0x11); //Sleep out
  HAL_Delay(120);              //Delay 120ms
  //************* Start Initial Sequence **********//
  nv3030b_write_index(0x36);
  NV3030B_SetRotation(nv3030b_display_dir);

  nv3030b_write_index(0x3A);
  spi_write_8bit(0x05);

  nv3030b_write_index(0xB2);
  spi_write_8bit(0x0C);
  spi_write_8bit(0x0C);
  spi_write_8bit(0x00);
  spi_write_8bit(0x33);
  spi_write_8bit(0x33);

  nv3030b_write_index(0xB7);
  spi_write_8bit(0x35);

  nv3030b_write_index(0xBB);
  spi_write_8bit(0x32); //Vcom=1.35V

  nv3030b_write_index(0xC2);
  spi_write_8bit(0x01);

  nv3030b_write_index(0xC3);
  spi_write_8bit(0x15); //GVDD=4.8V  颜色深度

  nv3030b_write_index(0xC4);
  spi_write_8bit(0x20); //VDV, 0x20:0v

  nv3030b_write_index(0xC6);
  spi_write_8bit(0x0F); //0x0F:60Hz

  nv3030b_write_index(0xD0);
  spi_write_8bit(0xA4);
  spi_write_8bit(0xA1);

  nv3030b_write_index(0xE0);
  spi_write_8bit(0xD0);
  spi_write_8bit(0x08);
  spi_write_8bit(0x0E);
  spi_write_8bit(0x09);
  spi_write_8bit(0x09);
  spi_write_8bit(0x05);
  spi_write_8bit(0x31);
  spi_write_8bit(0x33);
  spi_write_8bit(0x48);
  spi_write_8bit(0x17);
  spi_write_8bit(0x14);
  spi_write_8bit(0x15);
  spi_write_8bit(0x31);
  spi_write_8bit(0x34);

  nv3030b_write_index(0xE1);
  spi_write_8bit(0xD0);
  spi_write_8bit(0x08);
  spi_write_8bit(0x0E);
  spi_write_8bit(0x09);
  spi_write_8bit(0x09);
  spi_write_8bit(0x15);
  spi_write_8bit(0x31);
  spi_write_8bit(0x33);
  spi_write_8bit(0x48);
  spi_write_8bit(0x17);
  spi_write_8bit(0x14);
  spi_write_8bit(0x15);
  spi_write_8bit(0x31);
  spi_write_8bit(0x34);
  nv3030b_write_index(0x21);

  nv3030b_write_index(0x29);
#endif
}
