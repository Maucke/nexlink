---
name: usb-composite-debug
description: STM32 USB 复合设备（CDC + NEX_LINK）+ Windows MS OS 描述符调试。当处理 STM32 USB 复合设备枚举失败、WinUSB 接口连不上上位机、MS OS 1.0/2.0 描述符、libusb open -5、GUID 注册等 USB 问题时使用。
---

# STM32 USB 复合设备 + Windows MS OS 调试

本 skill 沉淀了 2026-08-10 在 `nexlink_template`（STM32F407 + FreeRTOS，USB HS + ULPI PHY）上做"CDC 虚拟串口 + NEX_LINK 复合设备"的完整调试经验。包括：复合设备的构建、MS OS 描述符的正确实现、Windows 10 25H2 的关键行为、以及高效调试工具链。

## 何时使用

- STM32 USB 设备枚举失败（Windows 代码 10/43、bus 上看不到设备）
- 复合设备（多个 USB 接口）的构建与端点分配
- WinUSB 接口注册 GUID 失败，上位机（libusb）连不上
- MS OS 1.0（0xEE / compatible ID / extended properties）与 MS OS 2.0（BOS / platform capability）配置
- 排查 libusb `open` 返回 -5（NOT_FOUND）或 -3（ACCESS）

## 关键背景（这个工程）

- 芯片 STM32F407，USB 走 **OTG_HS + ULPI 外部 PHY**（宏 `HUSB`，DEVICE_HS）
- USB 栈是 ST 官方 USBD（usbd_core/usbd_ctlreq/usbd_ioreq）
- 复合设备 = **CDC 虚拟串口（classId 0）+ NEX_LINK 厂商接口（classId 1）**
- 端点分配（定死，不要改）：NEX_LINK 0x81(IN)/0x01(OUT)，CDC 0x82(notif)/0x83(IN)/0x03(OUT)
- 注册顺序有硬依赖：**必须 CDC 先、NEX_LINK 后**（NEX_LINK 类靠"最后 Init"让 pClassData 指向自己的句柄）
- 上位机 pc-sdk/NexLink 用 libusb（nexlink_usb.dll），靠 WinUSB 接口 GUID `{c15b4308-04d3-11e6-b3ea-6057189e6443}` 发现设备

## 复合设备的构建（USE_USBD_COMPOSITE）

1. `usbd_conf.h`：定义 `USE_USBD_COMPOSITE`、`USE_USB_HS`、`USBD_MAX_NUM_INTERFACES=3`、`USBD_MAX_POWER=0x4B`
2. 新增 `usbd_composite_builder.c/h`（放 USB Core 中间件目录）：
   - `USBD_CMPSIT_TypeDef` 含 AddClass/GetHSConfigDescriptor/GetFSConfigDescriptor/GetOtherSpeedConfigDescriptor/GetDeviceQualifierDescriptor
   - `USBD_CMPSIT_AddClass`：取各类 `GetHSConfigDescriptor`，跳过 9B config 头，遍历子描述符：接口→重写 `bInterfaceNumber` 为递增序号 + 记入 `tclasslist[].Ifs[]`；端点→原样拷贝 + 记入 `Eps[]`；IAD→重写 `bFirstInterface`；其余原样拷贝。置 `tclasslist[].Active=1`
   - `GetXXXConfigDescriptor` 回填 `wTotalLength`/`bNumInterfaces`
3. CDC 描述符函数要**移出 `#ifndef USE_USBD_COMPOSITE` 守卫**（复合模式下类结构体里的 GetHS/FSConfigDescriptor 必须非 NULL，供 builder 解析）
4. `main.c` 注册顺序：
   ```c
   USBD_CDC_RegisterInterface(&hUSB, &USBD_Interface_fops_HS); // classId=0 前，写 pUserData[0]
   USBD_RegisterClassComposite(&hUSB, &USBD_CDC, CLASS_TYPE_CDC, NULL);
   USBD_RegisterClassComposite(&hUSB, &USBD_NEX_LINK, CLASS_TYPE_NONE, NULL);
   ```
5. **HS Tx FIFO 重分配**（usbd_conf.c）：复合后有 4 个 IN EP（EP0/1/2/3），要给每个 IN EP 配 Tx FIFO。HS 总 FIFO 4KB=1024 字，我的分配：RX 0x200 + TX0 0x80 + TX1 0x80 + TX2 0x20 + TX3 0x80 = 928 字
6. **CDC 需要 IAD（接口关联描述符）**：在 CDC config 描述符里加 `0x08 0x0B ...`（bFirstInterface, bInterfaceCount=2, class 02/02），否则 Windows usbser 无法把 comm+data 两个接口配对成串口（会报代码 10/28）
7. 不用 `usb_device.c`（CubeMX 生成的多类注册文件），直接在 `main.c` 的 `MX_USB_DEVICE_Init` 里注册

## MS OS 描述符（Windows 自动装驱动/注册 GUID 的关键）

### MS OS 1.0（0xEE 字符串 + vendor 请求）

实现（在 NEX_LINK 类）：
- `USBD_NEX_LINK_WINUSB_STR`：0xEE 字符串描述符（"MSFT100" + vendor code 0x20），在 `GetStrDesc` 里响应 index 0xEE
- `USBD_MS_COMP_ID_FEATURE_DESC`：vendor 请求 wIndex=0x0004，返回 compatible ID（"WINUSB"）
- `USBD_MS_EXT_PROP_FEATURE_DESC`：vendor 请求 wIndex=0x0005，返回 DeviceInterfaceGUIDs 属性
- 在 `USBD_NEX_LINK_CustomDeviceRequest` 里处理 0x0004/0x0005

**关键教训（踩过的坑）**：
- 复合设备里 NEX_LINK 是**接口 2**，compatible ID 描述符里的接口号字节要改成 2
- `0x0005` 处理里**不要判断 `wValue==0`**（原版单接口时接口是 0 所以能过；复合后 Windows 请求时 wValue=2，判断会 false → GUID 不注册）。直接无脑返回 GUID 即可（因为只有 NEX_LINK 处理 vendor 请求）
- **device class 必须保持 0/0/0**！不要声明 IAD（EF/02/01）——声明 IAD 后 Windows 25H2 不再请求 0x0005，GUID 注册不了

### MS OS 2.0（BOS / platform capability）——在 ST 栈上不可用

尝试过（失败）：
- device descriptor `bcdUSB=0x0210` + BOS 描述符（platform capability bLength=0x1C，UUID D8DD60DF-4589-4CC7-9CD2-659D9E648A9F，CapabilityData 8字节：dwWindowsVersion 0x06030000 + wMSOSDescriptorSetLength 0x00B2 + vendor code + alt enum）
- MS OS 2.0 descriptor set（178 字节 = 0x00B2）
- vendor 请求 wIndex=0x0007 返回描述符集
- 去掉 usbd_core.c `USBD_LL_DataInStage` 的 EP0 IN stall

**结论**：ST 官方 USBD 栈 + bcdUSB=0x0210 会卡枚举（Windows 请求 BOS 后不继续），无论 BOS 内容/device class/是否去 stall。ESP32 用 TinyUSB 栈能工作，但 ST 栈不行。**不要在 ST 栈上做 MS OS 2.0**。

### 正确方案（已验证成功）

**ST 栈 + MS OS 1.0 + device class=0/0/0（不声明 IAD）**，复合设备能正常枚举 + 注册 GUID + libusb 连接。

## Windows 10 25H2 的关键行为

1. **≥1809 忽略 MS OS 1.0 的 extended properties（0x0005 GUID）**，但**仍认 compatible ID**（0x0004，所以接口能自动绑 winusb.sys）
2. 但实测：**device class=0/0/0 时，25H2 会请求 0x0005**（GUID 能注册）；**声明 IAD（EF/02/01）后不再请求 0x0005**
3. 单接口设备（原版无串口）MS OS 1.0 能注册 GUID；复合设备只要 class=0/0/0 也能
4. 复合设备 CDC 必须配 IAD 描述符（config 里的 0x0B），否则 usbser 配对失败

## 高效调试工具链

### ST-Link CLI（免 IDE 烧录/读内存/复位）
```
"C:/Program Files (x86)/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe"
-c SWD -P <hex> -NoPrompt   # 烧录
-c SWD -Q -HardRst          # 硬复位
-c SWD HOTPLUG -r32 <addr> <words>   # 读内存（HOTPLUG 不打扰运行）
-c SWD HOTPLUG -r8 <addr> <bytes>
```
**重要**：`-c SWD`（Normal）每次会复位 MCU；读内存必须用 `-c SWD HOTPLUG`，否则每次读都复位设备。

### 关键 OTG HS 寄存器（注意地址，别读错偏移）
- GUSBCFG @0x4004000C（bit7 PHYSEL=0 外部 ULPI，bit29 ULPIFSLS）
- GCCFG @0x40040038（bit0 PWRDWN=0 PHY 供电，bit21 NOVBUSSENS）
- GINTSTS @0x4004001C（bit12 USBRST, bit13 ENUMDNE, bit11 DISCINT）
- DSTS @0x40040808（bit1:0 ENUMSPD：00=HS, 01=FS, 10=LS；bit0 SUSPSTS）
- DCTL @0x40040804（bit0 CGINN 软断开, bit1 SGIN 软连接）

### 注册表检查设备 GUID/驱动
```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USB\VID_1D51&PID_XXXX&MI_02\<instance>\Device Parameters
  DeviceInterfaceGUIDs = ['{c15b4308-...}']   ← 有则 libusb 能找到
  Service = WINUSB                            ← 绑了 winusb 驱动
```

### SetupAPI 枚举接口 GUID（验证 GUID 是否真注册）
用 ctypes 调 SetupDiGetClassDevsW/SetupDiEnumDeviceInterfaces，枚举 GUID_DEVINTERFACE_WINUSB (cee9e1d0) 和自定义 GUID。

### libusb 枚举设备
用 ctypes 调 libusb_get_device_list + libusb_get_device_descriptor，能确认设备是否在总线上、PID/bcdUSB/class。

### Bus Hound
抓 USB 总线控制传输，看 Windows 发了哪些 GET_DESCRIPTOR/vendor 请求、设备响应字节。定位 MS OS 请求（0x20/0x0004/0x0005/0x0007）是否到达。

### Windows 设备管理（避免翻车）
- **别用 `Disable-PnpDevice` 禁 USB 设备**——非管理员无法恢复，设备变 CM_PROB_PHANTOM，`pnputil /scan-devices` 需管理员。恢复要靠重新插拔 USB 线
- 改 PID 后两位（0x606d→0x606e）可强制 Windows 生成全新设备实例，避免旧驱动缓存干扰——**每次改固件验证时建议改 PID**

## 常见问题速查

| 症状 | 原因 | 解决 |
|------|------|------|
| Windows 代码 10/43，bus 看不到设备 | device class 声明了 IAD（EF/02/01）+ ST 栈 | 改回 class=0/0/0 |
| CDC 串口没有/两个接口配对失败 | config 缺 IAD 描述符 | 加 `08 0B ...` IAD |
| MI_02 绑了 WINUSB 但没 GUID | Windows ≥1809 忽略 MS OS 1.0 extended properties；或 class=EF/02/01 | class=0/0/0 + 0x0005 无脑返回 GUID |
| libusb open -5 (NOT_FOUND) | 接口没注册 GUID，libusb 找不到 | 注册 DeviceInterfaceGUIDs |
| libusb open -3 (ACCESS) | GUID 有了，但接口被占用/需 claim | 检查是否被其他进程打开 |
| 设备枚举 FS 不是 HS | ULPI PHY 问题 / 时钟 / FIFO | 读 GUSBCFG PHYSEL、DSTS ENUMSPD 排查 |
| 加 bcdUSB=0x0210 后卡枚举 | ST 栈 BOS 响应问题 | 保持 0x0200，不要用 MS OS 2.0 |

## 上位机 PC 端（pc-sdk）

- `NexLinkManager.Scan()` → `nexlink_usb.dll` 的 `usb_scan`，匹配 `(idProduct & 0xFF00)==0x6000`
- `nexlink_open` 按 serial 找设备，自动找 vendor 接口（LIBUSB_CLASS_VENDOR_SPEC + 有 IN/OUT bulk EP）
- libusb 在 Windows 上靠 WinUSB 接口 GUID 找设备；接口没 GUID 就 open -5
- 调试时可直接用 ctypes 调 libusb-1.0.dll 枚举/打开，不依赖 NexLink 库

## 验证清单

1. 设备管理器：MI_00 (usbser/COM 口) + MI_02 (WinUsb Device) 都 OK
2. 注册表 MI_02 有 DeviceInterfaceGUIDs
3. SetupAPI 枚举 c15b4308 GUID 能看到设备路径
4. NexLink_Tool / NexLink_Test 能 Scan 到设备
5. COM 口能收发（CDC 串口）
