#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nexlink_proto.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief   协议层入口：喂入从 USB/RingBuf 拿到的原始字节流
     *
     * @param data  字节数据指针
     * @param len   数据长度
     *
     * 调用场景：
     *   - NexLinkRxTask 中
     *   - 从 ringbuf_read() 取出数据后调用
     *
     * 特点：
     *   - 支持粘包 / 拆包
     *   - 内部维护解析状态
     *   - 自动分发 CMD
     */
    void nexlink_rx_bytes(const uint8_t *data, uint16_t len);

    void nexlink_log(const char *fmt, ...);

    /**
     * @brief   事件闸门
     *
     * 收到第一条合法命令（上位机已连上）之前、以及主机长时间不读（发送卡住）时，
     * 不允许主动推事件：主机没在读的时候推出去只会挂在 USB 上，把发送队列和
     * 缓冲池占满，最终连命令应答都发不出去。应答（send_resp）不受闸门影响。
     */
    bool nexlink_event_allowed(uint16_t cmd);
    void nexlink_event_disable(void);

    /**
     * @brief   上位机是否已经连上过
     *
     * 收到过第一条合法命令后就为 true（上位机连上就会先发 CMD_GET_VERSION），
     * USB 掉线/去配置时清回 false。应用侧可以用它把"等待连接"这类提示撤掉。
     */
    bool nexlink_host_connected(void);

    /**
     * @brief   应用侧实现的显示回调：把一块 RGB565 画到屏幕
     *
     * 收满一整行 CMD_FRAME_DATA 时由协议层回调（每次一行）：
     * 区间是 [x1,x2) x [y1,y2)，rgb565 指向 (x2-x1)*(y2-y1) 个像素、行优先。
     * 指针只在回调期间有效，实现里要同步画完（或自己拷走）。
     *
     * 组件带一份空实现（__weak），工程里不接屏幕也能编过。
     */
    void nexlink_display_flush(int x1, int y1, int x2, int y2, const uint8_t *rgb565);

    /**
     * @brief   图像接收统计
     *
     * 组件只累计、不打印（收帧路径里不做可能阻塞的串口输出）。应用侧周期性
     * 调 nexlink_frame_stat() 取走上一个窗口的数据（取走即清零），比如 1Hz
     * 打到 CDC 串口看 FPS / 实际速率 / 画图耗时。
     */
    typedef struct
    {
        uint32_t frames;        /* 窗口内收到的帧数 */
        uint32_t bytes;         /* 窗口内收到的像素字节数 */
        uint32_t draw_us;       /* 窗口内画图累计耗时 */
        uint32_t draw_calls;    /* 窗口内调用了多少次画图（SPI 传输次数） */
        uint32_t last_total_ms; /* 最近一帧：BEGIN→END 总耗时 */
        uint32_t last_draw_ms;  /* 最近一帧：画图耗时 */
        uint16_t last_w;        /* 最近一帧的尺寸 */
        uint16_t last_h;
    } nexlink_frame_stat_t;

    void nexlink_frame_stat(nexlink_frame_stat_t *out);

    /**
     * @brief   应用侧实现：报告屏幕信息
     *
     * 上位机连上后会发 CmdGetDisplayInfo(0x0011) 问设备屏幕尺寸/像素格式，
     * 用来把图片缩放到屏幕大小。返回 false 表示不支持（组件默认实现返回 false，
     * 此时上位机那边取不到屏幕信息、发图功能用不了）。
     */
    bool nexlink_display_info(nl_display_t *info);

#ifdef __cplusplus
}
#endif
