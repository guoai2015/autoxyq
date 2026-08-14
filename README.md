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

```powershell
# 按下 A 键 (0x04 = USB HID Usage ID)
.\autoxyq-cli.exe key 0x04

# 平滑移动鼠标 (dx dy 时长ms 轨迹类型)
.\autoxyq-cli.exe move 100 50 500 2

# 移动到屏幕绝对坐标
.\autoxyq-cli.exe moveto 800 600 300 2

# 鼠标左键按下 / 抬起
.\autoxyq-cli.exe button 1 down
.\autoxyq-cli.exe button 1 up

# 滚轮 (正值向上)
.\autoxyq-cli.exe scroll 3

# 配置随机延迟范围 (ms)
.\autoxyq-cli.exe delay 20 80
```

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
