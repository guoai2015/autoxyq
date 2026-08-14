# autoxyq — 虚拟键鼠输入合成库

基于 Windows `SendInput` 的用户态键鼠输入合成库，附带轨迹插值、随机延迟等行为模拟引擎。

> **后端说明**：项目原计划用 UMDF 2.x + vhidmini 做内核级虚拟 HID 驱动，但 UMDF 无法创建虚拟 HID 设备（VHF 是内核 API）。当前 `lib/` 已切换到 `SendInput` 后端；`driver/` 目录为未完成的 UMDF 骨架，默认不参与构建。

## 特性

- 键盘输入合成：USB HID 语义，6 键无冲 + 8 修饰键
- 鼠标输入合成：5 键 + 滚轮，相对位移 + 绝对坐标
- 轨迹插值引擎：三次贝塞尔 + Fitts' Law 修正 + 微抖动
- 随机延迟与按键节奏模拟
- 线程安全动作队列
- 零驱动、零签名、无需管理员权限

## 限制

`SendInput` 合成的输入带 `LLMHF_INJECTED` 标志，可被游戏反作弊识别。本项目仅用于学习与自动化测试；需要绕过反检测的场景请改用内核 VHF 驱动或硬件 HID 设备（见 `docs/approach-hardware-hid.md`）。

> **鼠标加速**：`move`（相对位移）受系统「提高指针精确度」影响，移动越快实际位移越大；`moveto` 使用 `MOUSEEVENTF_ABSOLUTE` 绝对坐标，落点精确，不受加速影响。
>
> **高 DPI**：库初始化时会声明 Per-Monitor V2 DPI 感知，因此 `moveto` 的坐标为**物理像素**，高 DPI 与混合缩放多显示器下均不受缩放比例影响。

## 系统要求

- Windows 10/11 x64

## 快速开始

### 本地构建

需要安装：

- Visual Studio 2022 或更新（含「使用 C++ 的桌面开发」工作负载）
- CMake 3.21+

```powershell
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

> 默认只构建 `lib` / `cli` / `tests`，无需 WDK。遗留的 `driver/` 需显式 `-DBUILD_DRIVER=ON` 才会构建（且需 WDK）。

产物：

- `build\cli\Release\autoxyq-cli.exe` — 命令行工具
- `build\lib\Release\autoxyq.dll` / `autoxyq-static.lib` — 动态库 / 静态库
- `build\tests\Release\*.exe` — 单元测试

### 运行单元测试

```powershell
cd build
ctest -C Release
```

### CLI 使用

通用形式：

```powershell
.\autoxyq-cli.exe <指令> <参数...>
```

执行每个指令会打印一行结果，行尾为可读错误描述；程序退出码等于最后操作的错误码（0 = 成功）。

#### 通用约定

- **数字解析**：除 `key` 的按键码外，所有数字参数按**十进制**解析。`key` 的按键码额外支持 `0x` 十六进制前缀（也支持 `0` 八进制前缀）。
- **坐标语义**：`move` 是相对位移，`moveto` 是绝对坐标（物理像素，见「坐标系统与 DPI」）。
- **修饰键限制**：`key` 仅支持普通按键（HID Usage ID `0x04`–`0x65`），**不支持** Ctrl / Shift / Alt / Win 等修饰键 —— 它们属于 HID Usage ID `0xE0`–`0xE7`，超出范围会返回 `Invalid parameter`。

#### 坐标系统与 DPI

- `move <dx> <dy>`：**相对位移**，单位像素，正 `x` 向右、正 `y` 向下。
- `moveto <x> <y>`：**绝对坐标**，单位**物理像素**，原点为虚拟屏幕左上角（多显示器时 `SM_XVIRTUALSCREEN` 可能为负，表示副屏在主屏左侧）。
- 库初始化时声明 Per-Monitor V2 DPI 感知，因此坐标不受系统缩放比例影响，高 DPI 与混合缩放多显示器下均精确。
- **鼠标加速**：`move`（相对位移）受系统「提高指针精确度」影响，移动越快实际位移越大；`moveto` 使用 `MOUSEEVENTF_ABSOLUTE` 绝对坐标，落点精确，不受加速影响。

#### 轨迹类型

| 值 | 名称 | 说明 |
|---|---|---|
| 0 | linear | 匀速直线 |
| 1 | bezier2 | 二次贝塞尔曲线 |
| 2 | bezier3 | 三次贝塞尔曲线（默认） |

#### 鼠标按键编号

| 编号 | 含义 |
|---|---|
| 1 | 左键 |
| 2 | 右键 |
| 3 | 中键 |
| 4 | 侧键 1（后退） |
| 5 | 侧键 2（前进） |

#### 指令详解

##### key — 键盘按键

```
autoxyq-cli key <usb_usage_id> [duration_ms]
```

| 参数 | 必填 | 说明 |
|---|---|---|
| usb_usage_id | 是 | 按键的 USB HID Usage ID（`0x04`–`0x65`），支持 `0x` 十六进制 |
| duration_ms | 否 | 按住时长（毫秒）。省略或 `0` = 瞬间点按（按下后立即弹起）；`> 0` = 按下 → 等待 duration_ms → 弹起 |

```powershell
.\autoxyq-cli.exe key 0x04          # 瞬间点按 A
.\autoxyq-cli.exe key 0x04 500      # 按住 A 500ms 后弹起
.\autoxyq-cli.exe key 0x28          # 瞬间点按 Enter
```

##### move — 平滑相对移动

```
autoxyq-cli move <dx> <dy> [duration_ms] [trajectory]
```

| 参数 | 必填 | 说明 |
|---|---|---|
| dx | 是 | X 轴相对位移（像素），正向右 |
| dy | 是 | Y 轴相对位移（像素），正向下 |
| duration_ms | 否 | 移动总时长（毫秒）。省略或 `0` = 瞬间移动 |
| trajectory | 否 | 轨迹类型（`0`/`1`/`2`），默认 `2` |

```powershell
.\autoxyq-cli.exe move 100 50        # 瞬间右移 100、下移 50
.\autoxyq-cli.exe move 100 50 500 2  # 500ms 内沿三次贝塞尔平滑移动
```

##### moveto — 移动到绝对坐标

```
autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]
```

| 参数 | 必填 | 说明 |
|---|---|---|
| x | 是 | 目标 X 绝对坐标（物理像素） |
| y | 是 | 目标 Y 绝对坐标（物理像素） |
| duration_ms | 是 | 移动总时长（毫秒）。`0` = 瞬移到目标 |
| trajectory | 否 | 轨迹类型（`0`/`1`/`2`），默认 `2` |

```powershell
.\autoxyq-cli.exe moveto 800 600 300 2  # 300ms 平滑移动到 (800,600)
.\autoxyq-cli.exe moveto 800 600 0      # 瞬移到 (800,600)
```

##### button — 鼠标按键

```
autoxyq-cli button <1-5> <down|up>
```

| 参数 | 必填 | 说明 |
|---|---|---|
| button | 是 | 按键编号 `1`–`5`（见「鼠标按键编号」表） |
| down/up | 是 | `down` 按下，`up` 弹起 |

```powershell
.\autoxyq-cli.exe button 1 down
.\autoxyq-cli.exe button 1 up
```

##### scroll — 滚轮

```
autoxyq-cli scroll <delta>
```

| 参数 | 必填 | 说明 |
|---|---|---|
| delta | 是 | 滚动量，正值向上、负值向下（单位格，`1` ≈ 一格，对应 `WHEEL_DELTA`=120） |

```powershell
.\autoxyq-cli.exe scroll 3    # 向上滚 3 格
.\autoxyq-cli.exe scroll -2   # 向下滚 2 格
```

##### delay — 设置随机延迟范围

```
autoxyq-cli delay <min_ms> <max_ms>
```

| 参数 | 必填 | 说明 |
|---|---|---|
| min_ms | 是 | 帧间随机延迟下限（毫秒） |
| max_ms | 是 | 帧间随机延迟上限（毫秒）。若 `min > max` 会自动交换 |

说明：`move`/`moveto` 沿轨迹逐帧发送，每帧之间的休眠时长在 `[min_ms, max_ms]` 内随机，用于模拟人类操作节奏。默认 `10`–`50`ms。

```powershell
.\autoxyq-cli.exe delay 20 80
```

##### reset — 抬起所有鼠标按键

```
autoxyq-cli reset
```

无参数。抬起全部 5 个鼠标按键（`1`–`5`），用于异常中断后恢复。

#### 键盘码表

`key` 指令的 `usb_usage_id` 取值（USB HID Keyboard/Keypad page）：

```text
字母键:
0x04 A, 0x05 B, 0x06 C, 0x07 D, 0x08 E, 0x09 F,
0x0A G, 0x0B H, 0x0C I, 0x0D J, 0x0E K, 0x0F L,
0x10 M, 0x11 N, 0x12 O, 0x13 P, 0x14 Q, 0x15 R,
0x16 S, 0x17 T, 0x18 U, 0x19 V, 0x1A W, 0x1B X,
0x1C Y, 0x1D Z

数字键:
0x1E 1, 0x1F 2, 0x20 3, 0x21 4, 0x22 5, 0x23 6,
0x24 7, 0x25 8, 0x26 9, 0x27 0

编辑/符号键:
0x28 Enter, 0x29 Escape, 0x2A Backspace, 0x2B Tab,
0x2C Space, 0x2D - _, 0x2E = +, 0x2F [ {, 0x30 ] },
0x31 \ |, 0x32 非美式 # ~, 0x33 ; :, 0x34 ' ", 0x35 ` ~,
0x36 , <, 0x37 . >, 0x38 / ?

功能键:
0x39 Caps Lock, 0x3A F1, 0x3B F2, 0x3C F3, 0x3D F4,
0x3E F5, 0x3F F6, 0x40 F7, 0x41 F8, 0x42 F9, 0x43 F10,
0x44 F11, 0x45 F12, 0x46 PrintScreen, 0x47 Scroll Lock,
0x48 Pause

导航键:
0x49 Insert, 0x4A Home, 0x4B Page Up, 0x4C Delete,
0x4D End, 0x4E Page Down, 0x4F Right Arrow, 0x50 Left Arrow,
0x51 Down Arrow, 0x52 Up Arrow

小键盘:
0x53 Num Lock, 0x54 Keypad /, 0x55 Keypad *, 0x56 Keypad -,
0x57 Keypad +, 0x58 Keypad Enter, 0x59 Keypad 1, 0x5A Keypad 2,
0x5B Keypad 3, 0x5C Keypad 4, 0x5D Keypad 5, 0x5E Keypad 6,
0x5F Keypad 7, 0x60 Keypad 8, 0x61 Keypad 9, 0x62 Keypad 0,
0x63 Keypad .

其他:
0x64 非美式 \ |, 0x65 菜单键 (Application)
```

#### 错误码

| 值 | 说明 |
|---|---|
| 0 | Success — 操作成功 |
| -1 | Driver not found — 未安装驱动（SendInput 后端下通常不会出现） |
| -2 | Device not ready — 设备未就绪 |
| -3 | IOCTL communication failed — IOCTL 通信失败 |
| -4 | Invalid parameter — 参数非法（如按键码越界、按键编号不在 1–5） |
| -5 | Action queue full — 队列或键盘缓冲已满 |
| -6 | Operation timed out — 操作超时 |

### 从 CI 下载预构建版本

前往 [Actions](https://github.com/guoai2015/autoxyq/actions) 页面下载最新 Artifact。

## 项目结构

```
autoxyq/
├── lib/             # 用户态控制库 (SendInput 后端 + 轨迹/延迟/队列)
├── cli/             # 命令行工具
├── driver/          # 遗留 UMDF 虚拟 HID 骨架 (未完成, 默认不构建)
├── tests/           # 单元测试
├── scripts/         # 驱动安装/卸载脚本 (仅遗留骨架用)
└── docs/            # 设计文档
```

## 安全说明

本工具仅供学习和自动化测试使用。严禁用于：

- 违反游戏服务条款的行为
- 任何违法或恶意用途

## License

MIT
