#pragma once
#include <stdint.h>

#define NL_MAGIC 0xA5

#define HEAD_LEN 8
#define NL_MAX_PAYLOAD 1024

#define NL_VERSION_MAJOR  1
#define NL_VERSION_MINOR  2
#define NL_VERSION_PATCH  0
#define NL_VERSION_BUILD  0

/* CMD */
//16-bit cmd = [ 高 8 位：功能域 ][ 低 8 位：子命令 ]
/* 0x0000 - core */
#define CMD_HW_RESET            0x0000
#define CMD_PING            0x0001
#define CMD_GET_VERSION     0x0002
#define CMD_SYNC_TIME       0x0003
#define CMD_LOOPBACK     0x0004
#define CMD_KEY     0x0010
#define CMD_GET_DISPLAY_INFO  0x0011

/* 0x0100 - log */
#define EVT_LOG             0x0100
#define EVT_WARN            0x0101
#define EVT_ERROR           0x0102

/* 0x0400 - frame */
#define CMD_FRAME_BEGIN     0x0400
#define CMD_FRAME_DATA      0x0401
#define CMD_FRAME_END       0x0402

#define CMD_FRAME_GET       0x0403

#define EVT_FRAME_UPLOAD_BEGIN    0x0404
#define EVT_FRAME_UPLOAD_DATA    0x0405
#define EVT_FRAME_UPLOAD_END    0x0406

typedef enum {
    NL_PKT_CMD   = 0x01,
    NL_PKT_RESP  = 0x02,
    NL_PKT_EVENT = 0x03
} nl_pkt_type_t;

typedef enum
{
    NL_ERR_OK            = 0x00,
    NL_ERR_UNSUPPORTED   = 0x01,
    NL_ERR_INVALID_PARAM = 0x02,
    NL_ERR_BUSY          = 0x03,
    NL_ERR_NOT_READY     = 0x04,
    NL_ERR_NO_MEM        = 0x05,
    NL_ERR_INTERNAL      = 0x7F,
} nl_err_t;

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  type;
    uint16_t cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t  payload[];
} nl_packet_t;

typedef struct __attribute__((packed))
{
    uint16_t err;   
    uint8_t  data[];
} nl_resp_t;

typedef struct __attribute__((packed))
{
    uint8_t  major;
    uint8_t  minor;
    uint16_t patch;
    uint32_t build;
} nl_version_t;

enum 
{
    Backspace = 8,
    Tab = 9,
    Clear = 12,
    Enter = 13,
    Pause = 19,
    Escape = 27,
    Spacebar = 32,
    PageUp = 33,
    PageDown = 34,
    End = 35,
    Home = 36,
    LeftArrow = 37,
    UpArrow = 38,
    RightArrow = 39,
    DownArrow = 40,
    Select = 41,
    Print = 42,
    Execute = 43,
    PrintScreen = 44,
    Insert = 45,
    Delete = 46,
    Help = 47,
    D0 = 48,
    D1 = 49,
    D2 = 50,
    D3 = 51,
    D4 = 52,
    D5 = 53,
    D6 = 54,
    D7 = 55,
    D8 = 56,
    D9 = 57,
    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    LeftWindows = 91,
    RightWindows = 92,
    Applications = 93,
    Sleep = 95,
    NumPad0 = 96,
    NumPad1 = 97,
    NumPad2 = 98,
    NumPad3 = 99,
    NumPad4 = 100,
    NumPad5 = 101,
    NumPad6 = 102,
    NumPad7 = 103,
    NumPad8 = 104,
    NumPad9 = 105,
    Multiply = 106,
    Add = 107,
    Separator = 108,
    Subtract = 109,
    Decimal = 110,
    Divide = 111,
    F1 = 112,
    F2 = 113,
    F3 = 114,
    F4 = 115,
    F5 = 116,
    F6 = 117,
    F7 = 118,
    F8 = 119,
    F9 = 120,
    F10 = 121,
    F11 = 122,
    F12 = 123,
    F13 = 124,
    F14 = 125,
    F15 = 126,
    F16 = 127,
    F17 = 128,
    F18 = 129,
    F19 = 130,
    F20 = 131,
    F21 = 132,
    F22 = 133,
    F23 = 134,
    F24 = 135,
    BrowserBack = 166,
    BrowserForward = 167,
    BrowserRefresh = 168,
    BrowserStop = 169,
    BrowserSearch = 170,
    BrowserFavorites = 171,
    BrowserHome = 172,
    VolumeMute = 173,
    VolumeDown = 174,
    VolumeUp = 175,
    MediaNext = 176,
    MediaPrevious = 177,
    MediaStop = 178,
    MediaPlay = 179,
    LaunchMail = 180,
    LaunchMediaSelect = 181,
    LaunchApp1 = 182,
    LaunchApp2 = 183,
    Oem1 = 186,
    OemPlus = 187,
    OemComma = 188,
    OemMinus = 189,
    OemPeriod = 190,
    Oem2 = 191,
    Oem3 = 192,
    Oem4 = 219,
    Oem5 = 220,
    Oem6 = 221,
    Oem7 = 222,
    Oem8 = 223,
    Oem102 = 226,
    Process = 229,
    Packet = 231,
    Attention = 246,
    CrSel = 247,
    ExSel = 248,
    EraseEndOfFile = 249,
    Play = 250,
    Zoom = 251,
    NoName = 252,
    Pa1 = 253,
    OemClear = 254
}ConsoleKey;

enum 
{
	Rgb888,
	Rgb565,
	Gray8,
	Mono1,
	Dual2Color
} TargetPixelFormat;

typedef struct __attribute__((packed))
{
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint8_t  refresh;
} nl_display_t;

#define MAX_DISPLAYS 1
typedef struct __attribute__((packed))
{
    uint8_t display_count;
    nl_display_t displays[MAX_DISPLAYS];
} nl_display_info_t;


