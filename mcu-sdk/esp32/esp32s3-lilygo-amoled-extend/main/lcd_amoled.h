#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LilyGO AMOLED 屏驱动（SH8601，QSPI 四线）。
 * 引脚 / 分辨率 / 镜像方向都在 main/config.h 里，本模块不暴露内部句柄。
 *
 * 坐标约定：左上角 (0,0)，x 向右、y 向下，范围 [0,width) x [0,height)。
 * 与 esp_lcd 一致，draw 的 x2/y2 是开区间。
 */

esp_err_t lcd_amoled_init(void);

int lcd_amoled_width(void);
int lcd_amoled_height(void);

/* 把一块 RGB565 像素（行优先，尺寸 (x2-x1)*(y2-y1)*2 字节）画到屏幕上。
   数据必须 DMA 可访问（内部 RAM 即可）。 */
esp_err_t lcd_amoled_draw(int x1, int y1, int x2, int y2, const void *rgb565);

/* 整屏填充一个 RGB565 颜色 */
esp_err_t lcd_amoled_fill(uint16_t rgb565);

/*
 * 画一行文字。内置的是一个"只包含常用字符"的极简 5x8 位图字体
 * （不是通用字体库，表里没有的字符会画成空白）。
 * 整个字符串会先渲染到一块缓冲，再用一次 SPI 传输送出去，所以不慢。
 *
 * @param scale 放大倍数（1 = 5x8 像素，2 = 10x16，3 = 15x24 ...）
 */
esp_err_t lcd_amoled_draw_text(int x, int y, const char *text, int scale,
                               uint16_t fg_rgb565, uint16_t bg_rgb565);

/* 在屏幕正中显示一行（水平垂直都居中） */
esp_err_t lcd_amoled_show_center_text(const char *text, int scale,
                                      uint16_t fg_rgb565, uint16_t bg_rgb565);

#ifdef __cplusplus
}
#endif
