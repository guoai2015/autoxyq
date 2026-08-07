# autoxyq - Virtual HID Keyboard & Mouse Driver

基于 UMDF 2.x + vhidmini 的 Windows 虚拟键鼠驱动程序，从驱动层模拟硬件级输入。

## 特性

- 虚拟 HID 键盘 (USB HID 启动键盘, 6 键无冲)
- 虚拟 HID 鼠标 (5 键 + 滚轮, 相对位移 + 绝对坐标)
- 轨迹插值引擎 (三次贝塞尔 + Fitts' Law 修正 + 微抖动)
- 随机延迟与按键节奏模拟
- 线程安全动作队列
- GitHub Actions CI/CD 自动构建

## 系统要求

- Windows 10/11 x64
- 管理员权限 (驱动安装时)

## 快速开始

### 从 CI 下载预构建版本

前往 [Actions](https://github.com/YOUR_USER/autoxyq/actions) 页面下载最新 Artifact。

### 本地构建

需要安装:
- Visual Studio 2022
- Windows Driver Kit (WDK)
- CMake 3.21+

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 安装驱动

```powershell
# 以管理员身份运行
.\scripts\install-driver.ps1
```

验证: 打开设备管理器，在 "人体学输入设备" 下应能看到 "autoxyq Virtual HID Device"。

### CLI 使用

```powershell
# 按下 A 键
.\autoxyq-cli.exe key 0x04

# 平滑移动鼠标
.\autoxyq-cli.exe move 100 50 500 2

# 移动到屏幕坐标
.\autoxyq-cli.exe moveto 800 600 300 2

# 鼠标左键按下
.\autoxyq-cli.exe button 1 down
.\autoxyq-cli.exe button 1 up

# 配置随机延迟
.\autoxyq-cli.exe delay 20 80
```

### 卸载驱动

```powershell
.\scripts\uninstall-driver.ps1
```

## 项目结构

```
autoxyq/
├── driver/          # UMDF 驱动
├── lib/             # 用户态控制库
├── cli/             # 命令行工具
├── tests/           # 单元测试
├── scripts/         # 安装/卸载脚本
├── docs/            # 设计文档
└── .github/workflows/  # CI/CD
```

## 安全说明

本工具仅供学习和自动化测试使用。严禁用于:
- 违反游戏服务条款的行为
- 任何违法或恶意用途

## License

MIT
