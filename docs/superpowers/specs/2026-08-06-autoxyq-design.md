# 虚拟键鼠驱动程序设计文档 (autoxyq)

> 日期: 2026-08-06
> 状态: 已确认，待实施

---

## 1. 项目概述

实现一个 Windows 虚拟键鼠程序，从驱动层模拟硬件级别的鼠标与键盘输入。目标是在使用内核反作弊的竞技类网游中，提供不可通过 `LLKHF_INJECTED` 标志被过滤的输入注入能力。

---

## 2. 技术选型

| 维度 | 决策 | 理由 |
|------|------|------|
| 平台 | Windows 10/11 x64 | 用户目标平台 |
| 驱动类型 | UMDF 2.x + vhidmini 代理 | 用户态驱动，不蓝屏，比 KMDF 安全；vhidmini 是微软官方示例，成熟稳定 |
| 语言 | C | 与 WDK 最自然契合 |
| 构建系统 | CMake + GitHub Actions | 用户无本地编译环境，CI 自动构建 |
| 签名 | CI 不签名；本地使用 Attestation Signing | CI 仅验证编译通过；正式使用前需签名 |
| 键盘 HID | USB HID 启动键盘，8 字节 Report，6 键无冲 | 标准实现，所有系统兼容 |
| 鼠标 HID | 5 键 + 滚轮，相对位移 + 绝对坐标双模式 | 相对模式最自然；绝对坐标保留备用 |
| 硬件备选方案 | Arduino/Teensy/RP2040 USB HID | 已评估，文档保存在 docs/approach-hardware-hid.md，待硬件就位后实施 |

---

## 3. 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     用户态 (User Mode)                           │
│                                                                 │
│  ┌─────────────────────┐    ┌──────────────────────────────┐   │
│  │   autoxyq-cli / API │───▶│   autoxyq-core (控制库)       │   │
│  │   (命令行 / 脚本接口) │    │   - 轨迹插值引擎             │   │
│  └─────────────────────┘    │   - 随机延迟引擎             │   │
│                              │   - 动作队列                │   │
│                              │   - IOCTL 通信层            │   │
│                              └───────────┬──────────────────┘   │
│                                          │ IOCTL                │
│                                          ▼                      │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              UMDF 驱动 (autoxyq-drv.dll)                   │  │
│  │             运行在 WUDFHost.exe 中                         │  │
│  │                                                           │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐   │  │
│  │  │ 虚拟 HID     │  │ 虚拟 HID     │  │ IOCTL 请求    │   │  │
│  │  │ 键盘设备     │  │ 鼠标设备     │  │ 处理器        │   │  │
│  │  └──────┬───────┘  └──────┬───────┘  └───────────────┘   │  │
│  │         │                 │                               │  │
│  │         └────────┬────────┘                               │  │
│  │                  │  HID Report                            │  │
│  └──────────────────┼────────────────────────────────────────┘  │
└─────────────────────┼───────────────────────────────────────────┘
                      │
┌─────────────────────┼───────────────────────────────────────────┐
│                 内核态 (Kernel Mode)                             │
│  ┌──────────────────▼───────────────────────────────────────┐  │
│  │                  HIDCLASS.sys (HID 类驱动)                │  │
│  │                 Windows 输入栈                             │  │
│  │                          │                                │  │
│  │                          ▼                                │  │
│  │                    游戏应用程序                            │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

键盘和鼠标作为两个独立的 HID 设备，而非一个 Composite Device。每个设备职责单一，符合真实外设的常见形态。

---

## 4. 项目结构

```
autoxyq/
├── .github/workflows/
│   └── build.yml                  # CI: 编译 driver + lib + cli + tests
├── driver/                         # UMDF 驱动 (C + WDK)
│   ├── CMakeLists.txt
│   ├── autoxyq-drv.c               # 驱动入口 (DriverEntry + EvtDriverDeviceAdd)
│   ├── autoxyq-drv.h
│   ├── hid-keyboard.c              # 虚拟键盘 HID 实现
│   ├── hid-mouse.c                 # 虚拟鼠标 HID 实现
│   ├── hid-report-desc.h           # HID Report 描述符常量
│   ├── ioctl-handler.c             # IOCTL 请求处理 (EvtIoDeviceControl)
│   ├── ioctl-defs.h                # IOCTL 命令码定义 (驱动+用户态共用)
│   ├── trace.h                     # WPP 跟踪宏
│   └── autoxyq-drv.inx             # INF 安装文件
├── lib/                            # 用户态控制库 (C)
│   ├── CMakeLists.txt
│   ├── autoxyq.h                   # 公开 API 头文件
│   ├── autoxyq.c                   # 库入口 (init / shutdown / reset)
│   ├── autoxyq-error.h             # 错误码定义
│   ├── ioctl-client.c              # IOCTL 客户端通信 (CreateFile / DeviceIoControl)
│   ├── trajectory.c                # 轨迹插值引擎 (贝塞尔 + Fitts' Law)
│   ├── trajectory.h
│   ├── delay.c                     # 随机延迟引擎
│   ├── delay.h
│   ├── action-queue.c              # 线程安全动作队列
│   └── action-queue.h
├── cli/                            # 命令行工具
│   ├── CMakeLists.txt
│   └── autoxyq-cli.c               # CLI 入口
├── tests/                          # 单元测试
│   ├── CMakeLists.txt
│   ├── test-trajectory.c
│   ├── test-delay.c
│   └── test-queue.c
├── scripts/
│   ├── install-driver.ps1          # 驱动安装脚本
│   └── uninstall-driver.ps1
├── docs/
│   ├── superpowers/specs/
│   │   └── 2026-08-06-autoxyq-design.md  # 本文件
│   └── approach-hardware-hid.md     # 硬件方案备选 (已评估)
├── CMakeLists.txt                   # 根 CMake
└── README.md
```

---

## 5. HID Report 描述符

### 5.1 虚拟键盘 — 启动键盘 (Boot Keyboard)

```
Usage Page:     0x01 (Generic Desktop)
Usage:          0x06 (Keyboard)

Report Size:    1 bit
Report Count:   8
Usage Page:     0x07 (Key Codes)
Usage Minimum:  0xE0 (Left Control)
Usage Maximum:  0xE7 (Right GUI)
→ 修饰键位图 (8 bit): LCtrl, LShift, LAlt, LGUI, RCtrl, RShift, RAlt, RGUI

Report Size:    1 bit
Report Count:   8
→ 保留位 (固定 0x00)

Report Size:    8 bit
Report Count:   6
Usage Page:     0x07 (Key Codes)
Usage Minimum:  0x00
Usage Maximum:  0x65
→ 最多 6 个同时按下的按键

总长度: 8 字节
```

按键编码使用 USB HID Usage ID (非 PS/2 Set 1)。

### 5.2 虚拟鼠标 — 5 键 + 滚轮

```
Usage Page:     0x01 (Generic Desktop)
Usage:          0x02 (Mouse)

Report Size:    1 bit
Report Count:   5
Usage Page:     0x09 (Button)
Usage Minimum:  1
Usage Maximum:  5
→ 按键位图: 左 / 右 / 中 / 侧键1 / 侧键2

Report Count:   3
→ 填充位

Report Size:    12 bit
Report Count:   2
Usage Page:     0x01
Usage:          0x30 (X), 0x31 (Y)
Logical Min/Max: -2047 / +2047
→ X 轴相对位移, Y 轴相对位移

Report Size:    8 bit
Report Count:   1
Usage:          0x38 (Wheel)
Logical Min/Max: -127 / +127
→ 垂直滚轮

总长度: 4 字节
```

鼠标默认使用相对位移模式，绝对坐标通过上层连续发送增量实现。

---

## 6. IOCTL 接口

| 命令 | 方向 | 输入结构 | 用途 |
|------|------|---------|------|
| `IOCTL_KEYBOARD_REPORT` | Input | `keyboard_report_t` | 发送一次键盘报告 |
| `IOCTL_MOUSE_REPORT` | Input | `mouse_report_t` | 发送一次鼠标报告 |
| `IOCTL_RESET_DEVICES` | Input | 无 | 重置所有设备（全零 Report） |

```c
// ioctl-defs.h

typedef struct {
    uint8_t modifier_bitmap;   // 8 位修饰键
    uint8_t reserved;          // 固定 0
    uint8_t key_codes[6];      // 最多 6 个按键
} keyboard_report_t;

typedef struct {
    uint8_t  button_mask;      // 5 位按键 (bit 0-4)
    int16_t  dx;               // X 位移 [-2047, 2047]
    int16_t  dy;               // Y 位移 [-2047, 2047]
    int8_t   wheel;            // 滚轮 [-127, 127]
} mouse_report_t;
```

---

## 7. 用户态公开 API

```c
// ---- 生命周期 ----
int autoxyq_init(void);       // 发现并打开驱动设备
void autoxyq_shutdown(void);  // 重置设备 + 关闭句柄

// ---- 键盘 ----
int autoxyq_key_down(uint8_t usb_usage_id);
int autoxyq_key_up(uint8_t usb_usage_id);
int autoxyq_key_press(uint8_t usb_usage_id, uint32_t duration_ms);

// ---- 鼠标（相对位移）----
int autoxyq_mouse_move(int16_t dx, int16_t dy);
int autoxyq_mouse_move_ex(int16_t dx, int16_t dy, uint32_t duration_ms,
                           enum trajectory_type type);

// ---- 鼠标（绝对坐标）----
int autoxyq_mouse_move_to(int x, int y, uint32_t duration_ms,
                           enum trajectory_type type);

// ---- 鼠标按键 ----
int autoxyq_mouse_button_down(uint8_t button);  // 1=左 2=右 3=中 4=侧1 5=侧2
int autoxyq_mouse_button_up(uint8_t button);
int autoxyq_mouse_scroll(int8_t delta);

// ---- 配置 ----
void autoxyq_set_delay_range(uint32_t min_ms, uint32_t max_ms);
void autoxyq_set_trajectory_type(enum trajectory_type type);
void autoxyq_set_jitter(uint8_t amplitude_px);   // 0 = 关闭微抖动

// ---- 错误处理 ----
const char* autoxyq_strerror(int err);
```

---

## 8. 轨迹插值引擎

### 8.1 处理流程

```
起点 (x₀, y₀) + 终点 (x₁, y₁) + 持续时间 T
        │
        ▼
  三次贝塞尔曲线路径生成
  (控制点带随机偏移，每次生成唯一路径)
        │
        ▼
  速度曲线建模 (加速 → 匀速 → 减速)
        │
        ▼
  Fitts' Law 修正 (接近目标时精度递增)
        │
        ▼
  微抖动叠加 (±amplitude px 高频噪声)
        │
        ▼
  N 个时间采样点 → 逐帧 IOCTL
```

### 8.2 贝塞尔模式

| 模式 | 说明 |
|------|------|
| `TRAJECTORY_LINEAR` | 直线 |
| `TRAJECTORY_BEZIER2` | 二次贝塞尔，单控制点随机偏移 |
| `TRAJECTORY_BEZIER3` | 三次贝塞尔，起始+结束方向，最自然（默认） |

### 8.3 Fitts' Law 修正

越接近终点，修正幅度越小——模拟人类"瞄准—微调—确认"的停止行为。在最后 10-15% 距离段，降速并增加小幅过冲/回退微调。

### 8.4 微抖动

在每个采样点上叠加 ±1~3 像素的高频低幅噪声，模拟手部自然颤抖。可通过 `autoxyq_set_jitter(0)` 关闭。

---

## 9. 随机延迟引擎

```
全局范围: autoxyq_set_delay_range(min_ms, max_ms)
  → 按键之间延迟:   [min_ms, max_ms] 均匀随机
  → 按下持续时长:   [min_ms, max_ms] 均匀随机
  → 鼠标帧间间隔:   [min_ms, max_ms] 均匀随机

精度: QueryPerformanceCounter 微秒级计时，对外毫秒级配置
单次覆盖: 部分 API 可传入特定延迟值覆盖全局配置
```

---

## 10. 动作队列

- 线程安全的生产者/消费者模型
- API 调用 → Push 到队列 → 工作线程消费 → IOCTL
- `autoxyq_flush()` 阻塞等待队列清空
- 队列满载时可选择阻塞等待或返回 `AUTOXYQ_ERR_QUEUE_FULL`

---

## 11. 驱动内部设计

### 11.1 设备初始化 (vhidmini 方案)

使用微软 WDK 示例 `src\hid\vhidmini`：
- vhidmini.sys: 内核代理驱动（微软提供，已有签名）
- autoxyq-drv.dll: UMDF 用户态驱动（本项目开发）
- 通过 vhidmini 提供的接口创建虚拟 HID 设备并提交 Report

初始化流程：
```
DriverEntry() → EvtDriverDeviceAdd() → 创建设备对象
   ├─ 创建 IOCTL 队列 (EvtIoDeviceControl)
   ├─ 通过 vhidmini 创建虚拟键盘 HID 设备
   ├─ 通过 vhidmini 创建虚拟鼠标 HID 设备
   └─ 导出设备接口 GUID
```

### 11.2 设备状态机

```
STOPPED ──(EvtDeviceD0Entry)──▶ RUNNING ──(EvtDeviceD0Exit)──▶ STOPPED
```

STOPPED 状态下 IOCTL 返回 `STATUS_DEVICE_NOT_READY`。

---

## 12. 错误处理

### 12.1 错误码

| 错误码 | 含义 |
|--------|------|
| `AUTOXYQ_OK` (0) | 成功 |
| `AUTOXYQ_ERR_DRIVER_NOT_FOUND` (-1) | 驱动未安装 |
| `AUTOXYQ_ERR_DEVICE_NOT_READY` (-2) | 设备未就绪 |
| `AUTOXYQ_ERR_IOCTL_FAILED` (-3) | IOCTL 通信失败 |
| `AUTOXYQ_ERR_INVALID_PARAM` (-4) | 参数非法 |
| `AUTOXYQ_ERR_QUEUE_FULL` (-5) | 动作队列已满 |
| `AUTOXYQ_ERR_TIMEOUT` (-6) | 操作超时 |

### 12.2 安全边界

- 用户态控制程序：无需管理员权限（运行时）
- 驱动安装：需要管理员权限（一次性）
- 驱动设备接口 SDDL：仅 Administrators + SYSTEM 可访问
- IOCTL 参数：驱动端严格校验，非法参数返回错误码

### 12.3 异常恢复

- `autoxyq_shutdown()`: 清空队列 → RESET 设备 → 关闭句柄
- 进程崩溃恢复: 重启进程 → `autoxyq_init` → 自动 RESET
- 第一版不做驱动端看门狗，保持简洁

---

## 13. 测试策略

| 层级 | 内容 | CI 自动化 |
|------|------|-----------|
| 单元测试 | trajectory / delay / action-queue 纯算法验证 | ✅ CI 中运行 |
| 集成测试 | IOCTL 通信 + 驱动安装/卸载 | ❌ 需驱动环境，手动执行 |
| 端到端测试 | 真实游戏/应用验证 | ❌ 人工验证 |

---

## 14. CI / CD (GitHub Actions)

- Runner: `windows-latest`
- 依赖: `microsoft/setup-wdk` + `ilammy/msvc-dev-cmd`
- 编译: CMake → MSBuild (x64 Release)
- 产物:
  - `autoxyq-drv/` — 驱动 DLL + INF + 安装脚本
  - `autoxyq-lib/` — 控制库 + 头文件
  - `autoxyq-cli.exe` — 命令行工具
- 测试: ctest 运行单元测试
- 签名: CI 产物的未签名版本，正式签名在本地完成
- 触发: push main / PR / workflow_dispatch
- Artifact 保留: 7 天

---

## 15. 待办 / 后续规划

- [ ] 硬件方案实施（待单片机就位，详见 docs/approach-hardware-hid.md）
- [ ] 驱动签名流程（Attestation Signing）
- [ ] 看门狗定时器（第二版，防止进程崩溃后按键卡住）
- [ ] 录制回放功能
- [ ] 支持更多 HID 设备类型 (Gamepad, Joystick)
