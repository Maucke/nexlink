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

static const char *TAG = "Nexlink";

extern TaskHandle_t nexlink_task_handle;
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
  ESP_LOGI(TAG, "TinyUSB started");
  ESP_LOGI(TAG, "log -> USB Initialized");
  xTaskCreatePinnedToCore(log_task, "log_task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
  NexLinkInit();
}
