# 方案二：硬件 HID 设备（待实施）

> **状态**: 已评估，待硬件就位后实施
> **适用场景**: 需要绕过内核反作弊的游戏自动化

## 架构

```
用户态控制程序 (行为模拟引擎)
        ↕ USB 串口 / 自定义协议
单片机固件 (ATmega32U4 / RP2040 / STM32)
        ↕ 原生 USB HID Report（物理 USB 线缆）
Windows USB HID 栈 → 应用程序（游戏）
```

## 推荐硬件

| 型号 | MCU | 价格 (¥) | USB HID | 备注 |
|------|-----|----------|---------|------|
| Arduino Pro Micro | ATmega32U4 | 15-25 | ✅ 原生 | 社区资源最多，Arduino 库成熟 |
| Teensy 4.0 | i.MX RT1062 | 150-200 | ✅ 原生 | 性能强，HID 报表速率高 |
| Raspberry Pi Pico | RP2040 | 25-35 | ✅ 原生 | TinyUSB 栈，C/C++ SDK |
| WeAct STM32F411 | STM32F4 | 30-50 | ✅ 原生 | 性能强，HAL 库 |

## 核心原理

1. 单片机在 USB 枚举时声明 Composite Device (HID Keyboard + HID Mouse)
2. Windows 将其识别为真实的 USB 键盘/鼠标，分配 HID 设备栈
3. PC 控制程序通过串口发送指令（如 `MOVE 100,50` / `KEY_DOWN A` / `KEY_UP A`）
4. 单片机解析指令，构造标准 HID Report 发送给 Host
5. 游戏/反作弊看到的是 USB 总线上的真实物理设备

## 反检测优势

- **物理不可区分**: 走标准 USB HID 协议，无驱动文件，无内核模块，无可枚举痕迹
- **无 INJECTED 标志**: HID Report 直接来自 USB 总线
- **可持续**: 不受微软签名政策变化影响，不受反作弊黑名单更新影响

## 待办

- [ ] 选购单片机硬件
- [ ] 固件开发 (USB HID 描述符 + 指令解析)
- [ ] PC 端控制库 (串口通信 + 行为模拟引擎)
- [ ] 轨迹插值与延迟配置
