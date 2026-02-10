# USB Device Bug Report Template

## 0. 标题
- 【类/功能】例如：UVC 枚举失败 / UAC 播放无声 / HID 报告长度异常 / NCM 不出网 / MSC 读写卡死
- 【平台】Windows 11 / Ubuntu 22.04 / Android / macOS / 嵌入式 Host
- 【连接形态】直连/Hub/Type-C 转接/长线

## 1. 现象（必填）
- 具体表现：
- 发生概率：必现/偶现（大约 xx%）
- 触发步骤（最小复现）：
  1)
  2)
  3)

## 2. 期望行为
- 你认为正确结果应该是什么：

## 3. 设备信息（必填）
- 芯片/USB IP：例如 STM32 / Synopsys DWC2 / DWC3 / 自研
- USB 速度：FS/HS/SS
- VID/PID：
- bcdDevice：
- 是否复合设备（Composite）：是/否
- 接口拓扑（简述）：例如 IAD + UVC(VC+VS) + UAC(AC+AS) + HID

## 4. 枚举/控制传输信息（强烈建议）
- 是否能拿到 Device Descriptor：是/否
- 是否能拿到 Configuration Descriptor：是/否
- 是否出现 STALL：是/否（在哪个 request）
- EP0 max packet：8/16/32/64
- 关键请求顺序（如果有抓包可省略）：

## 5. 日志（必填：二选一，最好两者都有）
### 5.1 设备端日志
- 固件版本/commit：
- 关键模块日志（ep0、class handler、urb/endpoint）：
- 日志文件路径（仓库内）：例如 debug/logs/2026-02-10_win11_uvc_enum_fail.txt

### 5.2 主机端日志
- Windows：设备管理器错误码 / USBView 信息 / SetupAPI.dev.log 片段
- Linux：dmesg / usbmon 解析摘要
- 文件路径（仓库内）：

## 6. 抓包（强烈建议）
- 工具：Wireshark/usbpcap/usbmon/硬件分析仪
- 抓包文件路径（仓库内）：例如 debug/pcaps/2026-02-10_win11_uvc_enum_fail.pcapng
- 备注：是否包含设备重插、是否从上电开始抓

## 7. 描述符与类特定信息（按需）
### UVC
- UVC 版本：1.0/1.1/1.5
- VS 格式：MJPEG/YUY2/H264...
- Probe/Commit 是否完成：是/否
- 相关日志/抓包位置：

### UAC
- UAC 版本：1.0/2.0
- 采样率/位宽/通道：
- Clock Source/Terminal/Feature Unit 拓扑：
- 相关日志/抓包位置：

### HID
- Report Descriptor 是否固定：是/否
- Report ID 使用情况：
- 相关日志/抓包位置：

### CDC NCM
- 是否出现 NCM 控制请求错误：
- 是否能拿到网络接口并获取 IP：
- 相关日志/抓包位置：

### MSC
- BOT/UASP（一般 BOT）：：
- SCSI 命令卡在哪个：INQUIRY/READ CAPACITY/READ(10)/WRITE(10)...
- 相关日志/抓包位置：

## 8. 你希望我优先回答的问题
- Q1：
- Q2：
