/*
 * NexLink CDC-Bridge（LilyGO AMOLED / esp32s3-lilygo-amoled-extend）
 *
 * 分工：
 *   - 屏幕：main/lcd_amoled.c（SH8601 QSPI，只管把像素画上屏）
 *   - 协议：components/nexlink4esp32（CMD/EVENT 解析、发送队列）
 *   - 这里只做接线：初始化屏幕 → 起日志任务 → NexLinkInit()
 *
 * 上位机发图的流程（pc-sdk NexLinkDevice.SendFrame / SendFrameFast）：
 *   CMD_FRAME_BEGIN(w,h,bpp) → 若干 CMD_FRAME_DATA → CMD_FRAME_END
 * 协议层按行拼齐后回调 nexlink_display_flush()，由 lcd_amoled.c 画到屏上。
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nexlink_tasks.h"
#include "nexlink_app.h"
#include "lcd_amoled.h"

static const char *TAG = "Nexlink";

/*
 * 每秒把图像接收统计打到 CDC 串口（FPS / 实际速率 / 画图耗时）。
 *
 * 注意：这里用 printf 而不是 ESP_LOGI —— esp_tusb_init_console() 是把 C 的
 * stdout/stderr 重定向到 CDC 的 VFS，ESP_LOG 走的是 console UART（本板 TX=GPIO39），
 * 不会出现在 CDC 那个串口上。
 *
 * 统计不在收帧路径里打印：CDC 没人读的时候写串口会卡住，放在收帧路径里会拖慢收包。
 */
static void log_task(void *arg)
{
  (void)arg;

  int64_t t_prev = esp_timer_get_time();
  uint32_t idle_ticks = 0;
  bool splash_shown = true; /* app_main 里已经画过"等待连接" */

  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* 上位机连上（收到过命令）就把提示清掉；掉线了再显示回来。
       第一次收图也会覆盖掉提示，这里只是让"连上但还没发图"这种情况也干净 */
    bool connected = nexlink_host_connected();
    if (connected && splash_shown)
    {
      lcd_amoled_fill(0x0000);
      splash_shown = false;
    }
    else if (!connected && !splash_shown)
    {
      lcd_amoled_show_center_text("Waiting for NexLink connection...", 2, 0xFFFF, 0x0000);
      splash_shown = true;
    }

    nexlink_frame_stat_t st;
    nexlink_frame_stat(&st);

    int64_t now = esp_timer_get_time();
    uint32_t dt_ms = (uint32_t)((now - t_prev) / 1000);
    t_prev = now;

    if ((st.frames > 0) && (dt_ms > 0))
    {
      uint32_t fps_x100 = (uint32_t)((uint64_t)st.frames * 100000ULL / dt_ms);
      uint32_t kbps = (uint32_t)((uint64_t)st.bytes * 1000ULL / 1024ULL / dt_ms);

      printf("[frame] %lu.%02lu fps, %lu KB/s, 画图 %luus/%lu 次; 上一帧 %ux%u 总 %lums（画图 %lums）\n",
             (unsigned long)(fps_x100 / 100), (unsigned long)(fps_x100 % 100),
             (unsigned long)kbps,
             (unsigned long)st.draw_us, (unsigned long)st.draw_calls,
             st.last_w, st.last_h,
             (unsigned long)st.last_total_ms, (unsigned long)st.last_draw_ms);
      fflush(stdout);

      /* 同一份数据也丢到 NEX_LINK 通道（工具日志面板）：CDC 串口那头要是没接上，
         还能在这里看到 */
      nexlink_log("[frame] %lu.%02lu fps, %lu KB/s, 画图 %luus/%lu 次, 上一帧 %ux%u 总 %lums\n",
                  (unsigned long)(fps_x100 / 100), (unsigned long)(fps_x100 % 100),
                  (unsigned long)kbps,
                  (unsigned long)st.draw_us, (unsigned long)st.draw_calls,
                  st.last_w, st.last_h, (unsigned long)st.last_total_ms);

      idle_ticks = 0;
    }
    else if ((++idle_ticks == 1) || (idle_ticks % 5 == 0))
    {
      /* 没有帧也要周期性吱一声：既能证明 CDC 串口是活的，
         也说明"没输出"是指令侧的问题（没发 CMD_FRAME_*）而不是串口的问题 */
      printf("[frame] idle，等待上位机发图（CMD_FRAME_BEGIN/DATA/END）\n");
      fflush(stdout);
    }

    UBaseType_t free_stack = uxTaskGetStackHighWaterMark(nexlink_task_handle);
    nexlink_log("nexlink_task_handle free stack: %u words\n", free_stack);
  }
}

void app_main(void)
{
  ESP_LOGI(TAG, "init amoled");
  ESP_ERROR_CHECK(lcd_amoled_init());
  lcd_amoled_fill(0x0000); /* 清屏 */

  /* 开机先在屏幕正中提示等待上位机；连上后由 log_task 撤掉。
     画提示失败不影响启动，所以只 warn 不 panic */
  if (lcd_amoled_show_center_text("Waiting for NexLink connection...", 2,
                                  0xFFFF /* 白 */, 0x0000 /* 黑 */) != ESP_OK)
    ESP_LOGW(TAG, "draw boot splash failed");

  ESP_LOGI(TAG, "TinyUSB started");
  ESP_LOGI(TAG, "log -> USB Initialized");
  xTaskCreatePinnedToCore(log_task, "log_task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
  NexLinkInit();
}
