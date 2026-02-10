# USB 调试资料采集清单（Device 端）

## A. 通用（任何类都尽量提供）
- [ ] 设备端固件 commit/version
- [ ] 复现步骤（最小化）
- [ ] 设备端日志：至少包含 EP0 setup 包解析、descriptor 返回、端点 enable/disable、stall 原因
- [ ] 主机端日志：Windows 错误码 / Linux dmesg
- [ ] 描述符导出：Device + Config（含所有 subordinate descriptors）
- [ ] 抓包：从重插/上电开始，直到问题发生

## B. Windows 抓包推荐
- usbpcap + Wireshark：尽量抓到 URB 控制传输与类请求
- 同时导出：
  - USBView（或 UVCView）输出
  - SetupAPI.dev.log 中对应设备安装段落（可截取）

## C. Linux 抓包推荐
- usbmon + Wireshark
- dmesg（从插入开始）

## D. 类别专项
### UVC
- [ ] VS Probe/Commit 过程是否完整（抓包能看出来）
- [ ] Streaming 开始后是否有 isoch/bulk 数据
- [ ] 若卡在枚举：重点看 IAD、VC/VS 接口号、端点属性、wTotalLength

### UAC
- [ ] 是否是 UAC1/UAC2
- [ ] Clock Source 与 AS 接口选择的采样率是否匹配
- [ ] Windows 下是否需要特定拓扑避免“感叹号/无法启动(代码10)”

### HID
- [ ] Report Descriptor（完整十六进制或文本）
- [ ] 报告长度与端点 maxPacket 是否匹配
- [ ] 是否用到了多个 Report ID

### CDC NCM
- [ ] NCM 功能描述符完整性
- [ ] NTB 参数、最大传输单元、通知端点是否正常
- [ ] 主机侧是否成功创建网卡并发 DHCP

### MSC
- [ ] BOT reset recovery 流程是否实现
- [ ] 是否正确处理 CBW/CSW、STALL 清除、sense 数据
