/**
 * This file is part of the NORDIX robotic platform firmware.
 *
 * Copyright (C) 2025 Valerii MAKAROV <v@nordix.dev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "nexlink_tasks.h"
#include "nexlink_app.h"
#include "esp_log.h"
#include "esp_lcd_panel_st7789.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include <stddef.h>
#include <stdio.h>

static const char *TAG = "Nexlink";

#define LCD_HOST SPI2_HOST

#define PIN_NUM_LCD_CS (GPIO_NUM_9)
#define PIN_NUM_LCD_PCLK (GPIO_NUM_10)
#define PIN_NUM_LCD_DATA0 (GPIO_NUM_11)
#define PIN_NUM_LCD_DATA1 (GPIO_NUM_12)
#define PIN_NUM_LCD_DATA2 (GPIO_NUM_13)
#define PIN_NUM_LCD_DATA3 (GPIO_NUM_14)
#define PIN_NUM_LCD_RST (GPIO_NUM_21)
#define PIN_NUM_BK_LIGHT (GPIO_NUM_NC)
#define PIN_NUM_LCD_POWER (GPIO_NUM_46)

#define LCD_BIT_PER_PIXEL (16)
#define LCD_OPCODE_WRITE_CMD (0x02ULL)

#define LCD_OPCODE_READ_CMD (0x03ULL)
#define LCD_OPCODE_WRITE_COLOR (0x32ULL)

#define TOUCH_MASTER_NUM I2C_NUM_1
#define TOUCH_SDA_NUM GPIO_NUM_47
#define TOUCH_SCL_NUM GPIO_NUM_48
#define TOUCH_RST_NUM GPIO_NUM_3
#define TOUCH_INT_NUM GPIO_NUM_45

typedef struct
{
  int cmd;               /*<! The specific LCD command */
  const void *data;      /*<! Buffer that holds the command specific data */
  size_t data_bytes;     /*<! Size of `data` in memory, in bytes */
  unsigned int delay_ms; /*<! Delay in milliseconds after this command */
} st7796_lcd_init_cmd_t;

typedef struct
{
  const st7796_lcd_init_cmd_t *init_cmds; /*!< Pointer to initialization commands array. Set to NULL if using default commands.
                                           *   The array should be declared as `static const` and positioned outside the function.
                                           *   Please refer to `vendor_specific_init_default` in source file.
                                           */
  uint16_t init_cmds_size;                /*<! Number of commands in above array */
} st7796_vendor_config_t;

st7796_lcd_init_cmd_t st7796_lcd_init_cmds[] = {
    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0xff, (uint8_t[]){0x22, 0x01, 0x01, 0x00}, 4, 0},
    {0x00, (uint8_t[]){0x80}, 1, 0},
    {0xff, (uint8_t[]){0x22, 0x01, 0x00}, 3, 0},
    {0x00, (uint8_t[]){0x90}, 1, 0},
    {0xc1, (uint8_t[]){0x1e, 0x1e}, 2, 0},
    {0x51, (uint8_t[]){0x00}, 1, 0},
    {0x00, (uint8_t[]){0x80}, 1, 0},
    {0xc0, (uint8_t[]){0x00, 0xf1, 0x00, 0x12, 0x00, 0x12, 0x00, 0xf1, 0x00, 0x12, 0x00, 0x12}, 12, 0},
    {0x00, (uint8_t[]){0x90}, 1, 0},
    {0xc0, (uint8_t[]){0x00, 0xf1, 0x00, 0x12, 0x00, 0x12, 0x00, 0xf1, 0x00, 0x12, 0x00, 0x12}, 12, 0},
    {0x00, (uint8_t[]){0xa1}, 1, 0},
    {0xb3, (uint8_t[]){0x78, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0}, 7, 0},
    {0x00, (uint8_t[]){0x82}, 1, 0},
    {0xb2, (uint8_t[]){0x66}, 1, 0},
    {0x00, (uint8_t[]){0x83}, 1, 0},
    {0xf3, (uint8_t[]){0x60, 0x80}, 2, 0},
    {0x00, (uint8_t[]){0x90}, 1, 0},
    {0xc2, (uint8_t[]){0x83, 0x01}, 2, 0},
    {0x00, (uint8_t[]){0x95}, 1, 0},
    {0xc2, (uint8_t[]){0xe5, 0x00, 0xdd}, 3, 0},
    {0x00, (uint8_t[]){0x98}, 1, 0},
    {0xc2, (uint8_t[]){0x82, 0x01, 0x01}, 3, 0},
    {0x00, (uint8_t[]){0x9d}, 1, 0},
    {0xc2, (uint8_t[]){0x98, 0x12, 0x00}, 3, 0},
    {0x00, (uint8_t[]){0xf0}, 1, 0},
    {0xc3, (uint8_t[]){0x83, 0x00, 0x00, 0x44, 0x00, 0xdb, 0x20}, 7, 0},
    {0x00, (uint8_t[]){0x0d}, 1, 0},
    {0xca, (uint8_t[]){0x05, 0x05, 0x05}, 3, 0},
    {0x00, (uint8_t[]){0x10}, 1, 0},
    {0xca, (uint8_t[]){0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40}, 15, 0},
    {0x00, (uint8_t[]){0xfa}, 1, 0},
    {0xc3, (uint8_t[]){0x7b, 0x0b}, 2, 0},
    {0x00, (uint8_t[]){0xb0}, 1, 0},
    {0xc2, (uint8_t[]){0x00, 0x02, 0x00, 0x8d, 0x00, 0x21}, 6, 0},
    {0x00, (uint8_t[]){0x80}, 1, 0},
    {0xc3, (uint8_t[]){0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00}, 11, 0},
    {0x00, (uint8_t[]){0x90}, 1, 0},
    {0xc3, (uint8_t[]){0x04, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0x04}, 11, 0},
    {0x00, (uint8_t[]){0xa0}, 1, 0},
    {0xc3, (uint8_t[]){0x16, 0x16, 0x16, 0x16, 0x15, 0x05, 0x06, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x04, 0x05, 0x06}, 16, 0},
    {0x00, (uint8_t[]){0xb0}, 1, 0},
    {0xc3, (uint8_t[]){0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x11, 0x09, 0x0a, 0x01, 0x02, 0x03, 0x16, 0x16, 0x16}, 16, 0},
    {0x00, (uint8_t[]){0xc0}, 1, 0},
    {0xc2, (uint8_t[]){0x32, 0x54, 0x10, 0x23, 0x45, 0x01, 0x35, 0x24, 0x01}, 9, 0},
    {0x00, (uint8_t[]){0xd0}, 1, 0},
    {0xc2, (uint8_t[]){0x32, 0x54, 0x10, 0x23, 0x45, 0x01, 0x35, 0x24, 0x01}, 9, 0},
    {0x00, (uint8_t[]){0xf0}, 1, 0},
    {0xc2, (uint8_t[]){0x00, 0x00, 0x02, 0x0f, 0x02, 0x0f, 0x02, 0x0f, 0x02, 0x0f, 0x02, 0x0f, 0x02, 0x0f}, 14, 0},
    {0x00, (uint8_t[]){0xa0}, 1, 0},
    {0xc0, (uint8_t[]){0x00, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x00, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a}, 15, 0},
    {0x00, (uint8_t[]){0x92}, 1, 0},
    {0xf5, (uint8_t[]){0x20}, 1, 0},
    {0x00, (uint8_t[]){0xe1}, 1, 0},
    {0xc2, (uint8_t[]){0x00}, 1, 0},
    {0x00, (uint8_t[]){0xe0}, 1, 0},
    {0xc3, (uint8_t[]){0x00, 0x11, 0x00, 0x11}, 4, 0},
    {0x00, (uint8_t[]){0x44}, 1, 0},
    {0xc5, (uint8_t[]){0xca}, 1, 0},
    {0x00, (uint8_t[]){0x40}, 1, 0},
    {0xc5, (uint8_t[]){0x29}, 1, 0},
    {0x00, (uint8_t[]){0x65}, 1, 0},
    {0xc4, (uint8_t[]){0xc0}, 1, 0},
    {0x00, (uint8_t[]){0x68}, 1, 0},
    {0xc4, (uint8_t[]){0x01}, 1, 0},
    {0x00, (uint8_t[]){0x14}, 1, 0},
    {0xc5, (uint8_t[]){0x12}, 1, 0},
    {0x00, (uint8_t[]){0x11}, 1, 0},
    {0xc5, (uint8_t[]){0x4a, 0x4a}, 2, 0},
    {0x00, (uint8_t[]){0xa1}, 1, 0},
    {0xc1, (uint8_t[]){0xc0, 0xe3}, 2, 0},
    {0x00, (uint8_t[]){0xa8}, 1, 0},
    {0xc1, (uint8_t[]){0x0a}, 1, 0},
    {0x00, (uint8_t[]){0xa8}, 1, 0},
    {0xc2, (uint8_t[]){0x54}, 1, 0},
    {0x00, (uint8_t[]){0x90}, 1, 0},
    {0xff, (uint8_t[]){0x80}, 1, 0},
    {0x00, (uint8_t[]){0x42}, 1, 0},
    {0xc5, (uint8_t[]){0x33, 0x44}, 2, 0},
    {0x00, (uint8_t[]){0x31}, 1, 0},
    {0xc5, (uint8_t[]){0xd6, 0xbb, 0xd6, 0xbb}, 4, 0},
    {0x00, (uint8_t[]){0x01}, 1, 0},
    {0xcb, (uint8_t[]){0x15}, 1, 0},
    {0x00, (uint8_t[]){0xd0}, 1, 0},
    {0xc0, (uint8_t[]){0x04}, 1, 0},
    {0x00, (uint8_t[]){0x02}, 1, 0},
    {0xc5, (uint8_t[]){0x05, 0xc5, 0x24, 0x24}, 4, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0xc5, (uint8_t[]){0x5b, 0x5b}, 2, 0},
    {0x00, (uint8_t[]){0x6c}, 1, 0},
    {0xf5, (uint8_t[]){0x00}, 1, 0},
    {0x00, (uint8_t[]){0x6b}, 1, 0},
    {0xc4, (uint8_t[]){0xb6}, 1, 0},
    {0x00, (uint8_t[]){0xf0}, 1, 0},
    {0xc0, (uint8_t[]){0x26}, 1, 0},
    {0x00, (uint8_t[]){0xf4}, 1, 0},
    {0xc0, (uint8_t[]){0x03}, 1, 0},
    {0x00, (uint8_t[]){0x86}, 1, 0},
    {0xb2, (uint8_t[]){0x49}, 1, 0},
    {0x00, (uint8_t[]){0x92}, 1, 0},
    {0xc4, (uint8_t[]){0xe0}, 1, 0},
    {0x00, (uint8_t[]){0x93}, 1, 0},
    {0xc4, (uint8_t[]){0x02}, 1, 0},
    {0x00, (uint8_t[]){0xa0, 0x00}, 2, 0},
    {0xb3, (uint8_t[]){0x00}, 1, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0xc8, (uint8_t[]){0xFF, 0x95, 0x78, 0x60, 0xFF, 0x48, 0x25, 0x08, 0xF0, 0xBF, 0xD8, 0xB1, 0x8E, 0x6B, 0xAA, 0x4C, 0x2F, 0x12, 0xF5, 0x6A, 0xD9, 0xBE, 0xA3, 0x95, 0x55, 0x86, 0x78, 0x6A, 0x5E, 0x55, 0x56, 0x54, 0x52, 0x15, 0xFF, 0x8B, 0x74, 0x5C, 0xFF, 0x47, 0x25, 0x08, 0xF1, 0xBF, 0xD9, 0xB3, 0x91, 0x70, 0xAA, 0x52, 0x35, 0x18, 0xFC, 0x6A, 0xE3, 0xC7, 0xAD, 0xA1, 0x55, 0x94, 0x86, 0x79, 0x6B, 0x55, 0x66, 0x63, 0x61, 0x15}, 64, 0},
    {0x00, (uint8_t[]){0x44}, 1, 0},
    {0xc8, (uint8_t[]){0xFF, 0x66, 0x48, 0x2F, 0xFF, 0x16, 0xF0, 0xD0, 0xB3, 0xAB, 0x97, 0x68, 0x40, 0x16, 0xAA, 0xF1, 0xCB, 0xA8, 0x84, 0x55, 0x62, 0x3E, 0x1A, 0x08, 0x55, 0xF7, 0xE5, 0xD5, 0xC3, 0x00, 0xB9, 0xB6, 0xB4, 0x00}, 34, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0xE8, (uint8_t[]){0xFF, 0xA7, 0x9D, 0x94, 0xFF, 0x87, 0x70, 0x60, 0x4E, 0xFF, 0x44, 0x34, 0x23, 0x13, 0xFF, 0x02, 0xF2, 0xE1, 0xD0, 0xAB, 0xC0, 0xB0, 0xA0, 0x98, 0xAA, 0x90, 0x88, 0x80, 0x78, 0xAA, 0x74, 0x71, 0x70, 0x2A, 0xFF, 0x9A, 0x93, 0x89, 0xFF, 0x82, 0x69, 0x5A, 0x4A, 0xFF, 0x42, 0x32, 0x22, 0x12, 0xFF, 0x02, 0xF2, 0xE2, 0xD2, 0xAB, 0xC2, 0xB2, 0xA2, 0x9A, 0xAA, 0x92, 0x8A, 0x82, 0x7A, 0xAA, 0x76, 0x74, 0x73, 0x2A}, 64, 0},
    {0x00, (uint8_t[]){0x44}, 1, 0},
    {0xE8, (uint8_t[]){0xFF, 0x7A, 0x70, 0x65, 0xFF, 0x58, 0x3F, 0x2E, 0x1A, 0xFF, 0x11, 0xFE, 0xEA, 0xD9, 0xAB, 0xC6, 0xB4, 0xA2, 0x8C, 0xAA, 0x7A, 0x67, 0x55, 0x4C, 0xAA, 0x42, 0x38, 0x2E, 0x26, 0xAA, 0x1F, 0x1A, 0x19, 0x2A}, 34, 0},
    // {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x36, (uint8_t[]){0x23}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    // {0x11, (uint8_t[]){0x00}, 0, 0},
    {0x29, (uint8_t[]){0x00}, 0, 10},
    {0x06, (uint8_t[]){0x01}, 1, 0}};

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 120
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY true

static void log_task(void *arg)
{
  (void)arg;
  while (1)
  {
    UBaseType_t free_stack = uxTaskGetStackHighWaterMark(nexlink_task_handle);
    nexlink_log("nexlink_task_handle free stack: %u words\n", free_stack);

    // ESP_LOGI(TAG, "log -> USB");
    // nexlink_log("log -> USB");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void app_main(void)
{

  if (PIN_NUM_LCD_POWER != GPIO_NUM_NC)
  {
    ESP_LOGI(TAG, "Enable amoled power");
    gpio_set_direction(PIN_NUM_LCD_POWER, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_LCD_POWER, 1);
  }
  spi_bus_config_t buscfg = {0};
  spi_device_handle_t spi_device = NULL;
  ESP_LOGI(TAG, "Initialize OLED SPI bus");
  buscfg.sclk_io_num = PIN_NUM_LCD_PCLK;
  buscfg.data0_io_num = PIN_NUM_LCD_DATA0;
  buscfg.max_transfer_sz = DISPLAY_WIDTH * 5 * sizeof(uint16_t);
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_handle_t panel_io = NULL;
  esp_lcd_panel_handle_t panel = NULL;
  // 液晶屏控制IO初始化
  ESP_LOGD(TAG, "Install panel IO");
  esp_lcd_panel_io_spi_config_t io_config = {};
  io_config.cs_gpio_num = PIN_NUM_LCD_CS;
  io_config.dc_gpio_num = PIN_NUM_LCD_DATA2;
  io_config.spi_mode = 0;
  io_config.pclk_hz = 40 * 1000 * 1000;
  io_config.trans_queue_depth = 1;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;

  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &panel_io));

  // 初始化液晶屏驱动芯片
  ESP_LOGD(TAG, "Install LCD driver");
  st7796_vendor_config_t vendor_config = {
      .init_cmds = st7796_lcd_init_cmds,
      .init_cmds_size = sizeof(st7796_lcd_init_cmds) / sizeof(st7796_lcd_init_cmd_t),
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
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  esp_lcd_panel_invert_color(panel, false);
  esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
  esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
  esp_lcd_panel_disp_on_off(panel, true);

  uint8_t data[1] = {((uint8_t)((255 * 100) / 100))};
  int lcd_cmd = 0x51;
  esp_lcd_panel_io_tx_param(panel_io, lcd_cmd, &data, sizeof(data));
// 填充整个屏幕为红色 (RGB565: 0xF800)
#define LCD_COLOR_RED 0xFFFF

  // 分配一行像素缓冲
  size_t line_pixels = DISPLAY_WIDTH; // 240
  uint16_t *line_buf = heap_caps_malloc(line_pixels * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (line_buf == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate line buffer");
    return;
  }
  for (size_t i = 0; i < line_pixels; i++)
  {
    line_buf[i] = LCD_COLOR_RED;
  }

  // 逐行刷屏
  for (int y = 0; y < DISPLAY_HEIGHT; y++)
  { // 120
    esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 1, line_buf);
  }

  free(line_buf);
  ESP_LOGI(TAG, "Screen filled with RED");
  // esp_lcd_touch_handle_t tp = nullptr;
  // ESP_LOGI(TAG, "Initialize I2C bus");
  // i2c_config_t conf = {
  //     .mode = I2C_MODE_MASTER,
  //     .sda_io_num = TOUCH_SDA_NUM,
  //     .scl_io_num = TOUCH_SCL_NUM,
  //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
  //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
  //     .master = {0},
  //     .clk_flags = 0,
  // };
  // conf.master.clk_speed = 400000;
  // i2c_bus_handle_t bus = i2c_bus_create(TOUCH_MASTER_NUM, &conf);
  // if (bus == nullptr)
  // {
  //   ESP_LOGW(TAG, "Initialize Touch I2C bus failed");
  //   return;
  // }
  // uint8_t device_addresses[256];
  // i2c_bus_scan(bus, device_addresses, 255);
  // touch_dev = i2c_bus_device_create(bus, 0x08, i2c_bus_get_current_clk_speed(bus));
  // if (touch_dev == nullptr)
  // {
  //   ESP_LOGW(TAG, "Add Touch I2C to bus failed");
  //   return;
  // }
  // button_custom_config_t custom_conf = {0};
  // custom_conf.active_level = 1;
  // custom_conf.button_custom_get_key_value = btn_home_get_key_value;
  // custom_conf.priv = touch_dev;

  // home_i2c_btn_ = new Button(custom_conf);
  // custom_conf.button_custom_get_key_value = btn_touch_get_key_value;
  // touch_i2c_btn_ = new Button(custom_conf);

  // touch_i2c_btn_->OnClick([this]()
  //                         {
  //           ESP_LOGI(TAG, "touch_i2c_btn_ pressed");
  //           power_save_timer_->WakeUp();
  //           auto& app = Application::GetInstance();
  //           if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
  //               ResetWifiConfiguration();
  //           }
  //           app.ToggleChatState(); });
  // home_i2c_btn_->OnClick([this]()
  //                        { ESP_LOGI(TAG, "home_i2c_btn_ pressed"); });
  ESP_LOGI(TAG, "TinyUSB started");
  ESP_LOGI(TAG, "log -> USB Initialized");
  xTaskCreatePinnedToCore(log_task, "log_task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
  NexLinkInit();
}
