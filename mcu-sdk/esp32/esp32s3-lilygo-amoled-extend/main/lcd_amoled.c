/*
 * LilyGO AMOLED（SH8601 QSPI 四线）屏驱动。
 *
 * 从 main.c 里分离出来：这里只管"把像素画上屏"，不涉及 NEX_LINK 协议。
 * 协议侧通过 nexlink_app.h 里声明的 nexlink_display_flush() 回调进来
 * （见文件末尾），这是全工程协议和屏幕的唯一连接点。
 */

#include "lcd_amoled.h"
#include "config.h"

#include <string.h>

#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_sh8601.h"
#include "nexlink_app.h"

static const char *TAG = "lcd";

#define LCD_BIT_PER_PIXEL (16)
#define LCD_OPCODE_WRITE_CMD (0x02ULL)

static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_line_buf; /* 一整行像素，DMA 可访问（fill 用） */

/* 面板初始化序列：先 sleep out，再设方向 / 像素格式 / 窗口 / 亮度 */
static const sh8601_lcd_init_cmd_t vendor_specific_init[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x36, (uint8_t[]){0xF0}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0}, // 16bits-RGB565
    {0x2A, (uint8_t[]){0x00, 0x00, 0x02, 0x17}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x00, 0xEF}, 4, 0},
    {0x29, (uint8_t[]){0x00}, 0, 10},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
};

int lcd_amoled_width(void)
{
    return DISPLAY_WIDTH;
}

int lcd_amoled_height(void)
{
    return DISPLAY_HEIGHT;
}

/* QSPI 下命令要带上 opcode（0x02 = write command），亮度 0x51 */
static void lcd_set_brightness(uint8_t percent)
{
    uint8_t level = (uint8_t)((255u * percent) / 100u);
    uint32_t cmd = ((uint32_t)LCD_OPCODE_WRITE_CMD << 24) | (0x51u << 8);

    esp_lcd_panel_io_tx_param(s_io, cmd, &level, 1);
}

esp_err_t lcd_amoled_init(void)
{
    if (PIN_NUM_LCD_POWER != GPIO_NUM_NC)
    {
        ESP_LOGI(TAG, "enable amoled power");
        ESP_RETURN_ON_ERROR(gpio_set_direction(PIN_NUM_LCD_POWER, GPIO_MODE_OUTPUT), TAG, "power gpio");
        ESP_RETURN_ON_ERROR(gpio_set_level(PIN_NUM_LCD_POWER, 1), TAG, "power on");
    }

    ESP_LOGI(TAG, "init qspi bus");
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = PIN_NUM_LCD_PCLK;
    buscfg.data0_io_num = PIN_NUM_LCD_DATA0;
    buscfg.data1_io_num = PIN_NUM_LCD_DATA1;
    buscfg.data2_io_num = PIN_NUM_LCD_DATA2;
    buscfg.data3_io_num = PIN_NUM_LCD_DATA3;
    buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
    buscfg.flags = SPICOMMON_BUSFLAG_QUAD;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi bus");

    ESP_LOGI(TAG, "install panel io");
    esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(PIN_NUM_LCD_CS, NULL, NULL);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &s_io), TAG, "panel io");

    ESP_LOGI(TAG, "install panel driver");
    sh8601_vendor_config_t vendor_config = {
        .init_cmds = &vendor_specific_init[0],
        .init_cmds_size = sizeof(vendor_specific_init) / sizeof(sh8601_lcd_init_cmd_t),
        .flags = {
            .use_qspi_interface = 1,
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .flags = {
            .reset_active_high = 0,
        },
        .vendor_config = &vendor_config,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_sh8601(s_io, &panel_config, &s_panel), TAG, "panel sh8601");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel init");
    esp_lcd_panel_invert_color(s_panel, false);
    /* 面板原生就是 536x240，不需要 swap_xy（config.h 里的 DISPLAY_SWAP_XY 不用） */
    esp_lcd_panel_mirror(s_panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
    esp_lcd_panel_disp_on_off(s_panel, true);

    lcd_set_brightness(100);

    s_line_buf = heap_caps_malloc(DISPLAY_WIDTH * sizeof(uint16_t), MALLOC_CAP_DMA);
    ESP_RETURN_ON_FALSE(s_line_buf != NULL, ESP_ERR_NO_MEM, TAG, "line buffer");

    ESP_LOGI(TAG, "amoled %dx%d ready", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
}

esp_err_t lcd_amoled_draw(int x1, int y1, int x2, int y2, const void *rgb565)
{
    if ((s_panel == NULL) || (rgb565 == NULL))
        return ESP_ERR_INVALID_STATE;

    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 > DISPLAY_WIDTH)
        x2 = DISPLAY_WIDTH;
    if (y2 > DISPLAY_HEIGHT)
        y2 = DISPLAY_HEIGHT;

    if ((x1 >= x2) || (y1 >= y2))
        return ESP_ERR_INVALID_ARG;

    return esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2, y2, rgb565);
}

esp_err_t lcd_amoled_fill(uint16_t rgb565)
{
    if ((s_panel == NULL) || (s_line_buf == NULL))
        return ESP_ERR_INVALID_STATE;

    for (int i = 0; i < DISPLAY_WIDTH; i++)
        s_line_buf[i] = rgb565;

    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, y, DISPLAY_WIDTH, y + 1, s_line_buf);
        if (err != ESP_OK)
            return err;
    }

    return ESP_OK;
}

/*
 * NEX_LINK 协议层（组件的 nexlink_app.c）把上位机传来的图像按行交过来。
 * 一次调用的是一整行到几行（x1=0, x2=width），像素数据在两块 ping-pong 缓冲
 * 之间轮换 —— 交出来的这一块在下次调用里不会被写，所以直接把指针给面板 IO 即可。
 */
void nexlink_display_flush(int x1, int y1, int x2, int y2, const uint8_t *rgb565)
{
    lcd_amoled_draw(x1, y1, x2, y2, rgb565);
}

/*
 * 上位机连上后会问屏幕信息（用来把图片缩放到屏幕大小），这里如实上报。
 * 注意：本驱动只吃 RGB565，所以 bpp 固定报 NL_BPP_RGB565。
 */
bool nexlink_display_info(nl_display_t *info)
{
    info->width = (uint16_t)lcd_amoled_width();
    info->height = (uint16_t)lcd_amoled_height();
    info->bpp = (uint8_t)NL_BPP_RGB565;
    info->refresh = 30; /* 只给上位机看，不代表实际帧率 */
    return true;
}

/* ================= 极简位图字体 + 文本绘制 =================
 *
 * 这个工程没有字体库（没引 LVGL），而状态提示只需要几个字符，所以内置一张
 * 5 列 x 8 行的小字库：直接画成 '#' 图案（比一堆十六进制好核对、改起来也直观），
 * 第 8 行留给 g 这种下伸部分，表里没有的字符按空白处理。
 */

#define LCD_GLYPH_W 5
#define LCD_GLYPH_H 8
#define LCD_GLYPH_ADV 6 /* 含 1 列字间距 */

typedef struct
{
    char ch;
    const char *rows[LCD_GLYPH_H];
} lcd_glyph_t;

static const lcd_glyph_t s_glyphs[] = {
    {' ', {"     ", "     ", "     ", "     ", "     ", "     ", "     ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", "     ", "  #  ", "     "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #", "     "}},
    {'N', {"#   #", "##  #", "##  #", "# # #", "#  ##", "#  ##", "#   #", "     "}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####", "     "}},
    {'a', {"     ", "     ", " ### ", "    #", " ####", "#   #", " ####", "     "}},
    {'c', {"     ", "     ", " ### ", "#   #", "#    ", "#   #", " ### ", "     "}},
    {'e', {"     ", "     ", " ### ", "#   #", "#####", "#    ", " ### ", "     "}},
    {'f', {"  ## ", " #   ", "#### ", " #   ", " #   ", " #   ", " #   ", "     "}},
    {'g', {"     ", "     ", " ####", "#   #", "#   #", " ####", "    #", " ### "}},
    {'i', {"  #  ", "     ", "  #  ", "  #  ", "  #  ", "  #  ", " ### ", "     "}},
    {'k', {"#    ", "#    ", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "     "}},
    {'m', {"     ", "     ", "## # ", "# # #", "# # #", "# # #", "# # #", "     "}},
    {'n', {"     ", "     ", "# ## ", "##  #", "#   #", "#   #", "#   #", "     "}},
    {'o', {"     ", "     ", " ### ", "#   #", "#   #", "#   #", " ### ", "     "}},
    {'r', {"     ", "     ", "# ## ", "##  #", "#    ", "#    ", "#    ", "     "}},
    {'t', {" #   ", " #   ", "#### ", " #   ", " #   ", " #  #", "  ## ", "     "}},
    {'x', {"     ", "     ", "#   #", " # # ", "  #  ", " # # ", "#   #", "     "}},
};

static const lcd_glyph_t *lcd_find_glyph(char ch)
{
    for (unsigned i = 0; i < sizeof(s_glyphs) / sizeof(s_glyphs[0]); i++)
    {
        if (s_glyphs[i].ch == ch)
            return &s_glyphs[i];
    }
    return NULL;
}

/* 面板是 RGB565 big-endian（和上位机 ImageBppConverter 的输出一致） */
static inline void lcd_put565(uint8_t *p, uint16_t c)
{
    p[0] = (uint8_t)(c >> 8);
    p[1] = (uint8_t)(c & 0xFF);
}

esp_err_t lcd_amoled_draw_text(int x, int y, const char *text, int scale,
                               uint16_t fg_rgb565, uint16_t bg_rgb565)
{
    if ((s_panel == NULL) || (text == NULL) || (scale < 1))
        return ESP_ERR_INVALID_ARG;

    int len = (int)strlen(text);
    if (len == 0)
        return ESP_OK;

    int w = len * LCD_GLYPH_ADV * scale;
    int h = LCD_GLYPH_H * scale;

    if ((x < 0) || (y < 0) || ((x + w) > DISPLAY_WIDTH) || ((y + h) > DISPLAY_HEIGHT))
        return ESP_ERR_INVALID_ARG;

    /* 先整段渲染到一块缓冲，最后一次性 SPI 送出（不是逐像素发） */
    uint8_t *buf = heap_caps_malloc((size_t)w * h * 2, MALLOC_CAP_DMA);
    if (buf == NULL)
        return ESP_ERR_NO_MEM;

    for (int i = 0; i < w * h; i++)
        lcd_put565(buf + (size_t)i * 2, bg_rgb565);

    for (int idx = 0; idx < len; idx++)
    {
        const lcd_glyph_t *g = lcd_find_glyph(text[idx]);
        if (g == NULL)
            continue;

        for (int r = 0; r < LCD_GLYPH_H; r++)
        {
            const char *row = g->rows[r];

            for (int c = 0; c < LCD_GLYPH_W; c++)
            {
                if (row[c] != '#')
                    continue;

                /* 一个点亮像素按 scale 放大成方块 */
                for (int dy = 0; dy < scale; dy++)
                {
                    int py = r * scale + dy;

                    for (int dx = 0; dx < scale; dx++)
                    {
                        int px = (idx * LCD_GLYPH_ADV + c) * scale + dx;
                        lcd_put565(buf + ((size_t)py * w + px) * 2, fg_rgb565);
                    }
                }
            }
        }
    }

    esp_err_t err = lcd_amoled_draw(x, y, x + w, y + h, buf);
    heap_caps_free(buf);
    return err;
}

esp_err_t lcd_amoled_show_center_text(const char *text, int scale,
                                      uint16_t fg_rgb565, uint16_t bg_rgb565)
{
    if (text == NULL)
        return ESP_ERR_INVALID_ARG;

    int w = (int)strlen(text) * LCD_GLYPH_ADV * scale;
    int h = LCD_GLYPH_H * scale;

    int x = (DISPLAY_WIDTH - w) / 2;
    int y = (DISPLAY_HEIGHT - h) / 2;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    return lcd_amoled_draw_text(x, y, text, scale, fg_rgb565, bg_rgb565);
}
