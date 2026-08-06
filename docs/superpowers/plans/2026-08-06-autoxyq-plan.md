# autoxyq 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个 Windows UMDF 虚拟 HID 键鼠驱动程序及配套用户态控制库，从驱动层模拟硬件级输入。

**Architecture:** UMDF 2.x 用户态驱动通过 vhidmini 代理创建虚拟 HID 键盘+鼠标设备，用户态控制库通过 IOCTL 与驱动通信，包含轨迹插值引擎、随机延迟引擎和动作队列。

**Tech Stack:** C 语言, UMDF 2.x (WDK), vhidmini, CMake, GitHub Actions (windows-latest)

## Global Constraints

- 平台: Windows 10/11 x64 only
- 语言: C (C99 或更高)
- 驱动类型: UMDF 2.x + vhidmini 代理
- 键盘 HID: USB HID 启动键盘, 8 字节 Report, 6 键无冲
- 鼠标 HID: 5 键 + 滚轮, 相对位移 (默认) + 绝对坐标双模式
- 轨迹引擎: 三次贝塞尔 + 随机偏移 + Fitts' Law 修正 + 微抖动
- 延迟引擎: 毫秒级可配置均匀随机延迟, QueryPerformanceCounter 计时
- 构建: CMake + MSBuild, x64 Release
- CI: GitHub Actions windows-latest + setup-wdk
- 签名: CI 中不签名; 本地签名流程
- 编码风格: KISS 原则, 驱动端不做防御式恢复, 错误返回错误码

---

### Task 1: 项目骨架搭建

**Files:**
- Create: `G:\ai\autoxyq\CMakeLists.txt`
- Create: `G:\ai\autoxyq\README.md`
- Create: `G:\ai\autoxyq\.gitignore`

**Interfaces:**
- Consumes: (none, first task)
- Produces: 项目根目录骨架, 子目录由后续任务创建各自文件时自动生成

- [ ] **Step 1: 创建根 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.21)
project(autoxyq VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 全局编译选项
if(MSVC)
    add_compile_options(/W4 /WX /wd4100 /wd4201 /wd4214)
endif()

# 子目录将在后续任务中逐步启用
# add_subdirectory(driver)
# add_subdirectory(lib)
# add_subdirectory(cli)
# add_subdirectory(tests)
```

- [ ] **Step 2: 创建 .gitignore**

```
build/
out/
*.user
*.suo
*.sdf
*.opensdf
*.vcxproj
*.vcxproj.filters
.vs/
x64/
Debug/
Release/
*.dll
*.exe
*.sys
*.pdb
*.ilk
*.exp
*.lib
*.obj
*.cert
*.pfx
```

- [ ] **Step 3: 创建 README.md (骨架)**

```markdown
# autoxyq - Virtual HID Keyboard & Mouse Driver

基于 UMDF 2.x 的 Windows 虚拟键鼠驱动程序。

## 构建

需要安装 Windows Driver Kit (WDK) 和 Visual Studio 2022。

### 本地构建

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### CI 构建

推送到 main 分支自动触发 GitHub Actions 构建。

## 安装驱动

```powershell
.\scripts\install-driver.ps1
```

## 卸载驱动

```powershell
.\scripts\uninstall-driver.ps1
```
```

- [ ] **Step 4: 创建目录结构**

```powershell
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\.github\workflows"
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\driver"
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\lib"
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\cli"
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\tests"
New-Item -ItemType Directory -Force -Path "G:\ai\autoxyq\scripts"
```

---

### Task 2: GitHub Actions CI 构建流水线

**Files:**
- Create: `G:\ai\autoxyq\.github\workflows\build.yml`

**Interfaces:**
- Consumes: 项目根目录结构 (Task 1)
- Produces: CI 自动构建, 上传 artifact

- [ ] **Step 1: 创建 build.yml**

```yaml
name: Build autoxyq

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  workflow_dispatch:

jobs:
  build:
    runs-on: windows-latest

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup WDK
        uses: microsoft/setup-wdk@v1

      - name: Setup MSVC
        uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: amd64

      - name: Configure CMake
        run: |
          mkdir build
          cd build
          cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: |
          cd build
          cmake --build . --config Release

      - name: Run Unit Tests
        run: |
          cd build
          ctest -C Release --output-on-failure

      - name: Package Artifacts
        run: |
          New-Item -ItemType Directory -Force -Path artifacts/driver
          New-Item -ItemType Directory -Force -Path artifacts/lib
          Copy-Item build/driver/Release/autoxyq-drv.dll artifacts/driver/ -ErrorAction SilentlyContinue
          Copy-Item build/driver/Release/autoxyq-drv.pdb artifacts/driver/ -ErrorAction SilentlyContinue
          Copy-Item driver/autoxyq-drv.inx artifacts/driver/
          Copy-Item scripts/install-driver.ps1 artifacts/driver/
          Copy-Item scripts/uninstall-driver.ps1 artifacts/driver/
          Copy-Item build/lib/Release/autoxyq.dll artifacts/lib/ -ErrorAction SilentlyContinue
          Copy-Item build/lib/Release/autoxyq.lib artifacts/lib/ -ErrorAction SilentlyContinue
          Copy-Item lib/autoxyq.h artifacts/lib/
          Copy-Item lib/autoxyq-error.h artifacts/lib/
          Copy-Item build/cli/Release/autoxyq-cli.exe artifacts/ -ErrorAction SilentlyContinue

      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: autoxyq-build
          path: artifacts/
          retention-days: 7
```

---

### Task 3: 共享定义 — IOCTL 命令码、错误码、HID 描述符

**Files:**
- Create: `G:\ai\autoxyq\driver\ioctl-defs.h`
- Create: `G:\ai\autoxyq\lib\autoxyq-error.h`
- Create: `G:\ai\autoxyq\driver\hid-report-desc.h`

**Interfaces:**
- Consumes: 项目结构 (Task 1)
- Produces:
  - `keyboard_report_t` 结构体
  - `mouse_report_t` 结构体
  - `IOCTL_KEYBOARD_REPORT`, `IOCTL_MOUSE_REPORT`, `IOCTL_RESET_DEVICES` 宏
  - `AUTOXYQ_DEVICE_INTERFACE_GUID` GUID 定义
  - 错误码枚举 `autoxyq_error`
  - HID Report 描述符字节数组常量

- [ ] **Step 1: 创建 ioctl-defs.h**

```c
#ifndef IOCTL_DEFS_H
#define IOCTL_DEFS_H

#include <stdint.h>
#include <initguid.h>

// 设备接口 GUID: {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
DEFINE_GUID(GUID_AUTOXYQ_DEVICE_INTERFACE,
    0xA1B2C3D4, 0xE5F6, 0x7890, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90);

// IOCTL 设备类型
#define FILE_DEVICE_AUTOXYQ  0x8000

// IOCTL 命令码
#define IOCTL_KEYBOARD_REPORT  CTL_CODE(FILE_DEVICE_AUTOXYQ, 0x800, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MOUSE_REPORT     CTL_CODE(FILE_DEVICE_AUTOXYQ, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_RESET_DEVICES    CTL_CODE(FILE_DEVICE_AUTOXYQ, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// 键盘输入报告
#pragma pack(push, 1)
typedef struct {
    uint8_t modifier_bitmap;   // 修饰键位图: LCtrl|LShift|LAlt|LGUI|RCtrl|RShift|RAlt|RGUI
    uint8_t reserved;          // 保留，固定为 0
    uint8_t key_codes[6];      // 同时按下的按键 (USB HID Usage ID)
} keyboard_report_t;

// 鼠标输入报告
typedef struct {
    uint8_t  button_mask;      // 按键位图 [bit0=左, bit1=右, bit2=中, bit3=侧1, bit4=侧2]
    int16_t  dx;               // X 轴相对位移，范围 [-2047, +2047]
    int16_t  dy;               // Y 轴相对位移，范围 [-2047, +2047]
    int8_t   wheel;            // 垂直滚轮，范围 [-127, +127]
} mouse_report_t;
#pragma pack(pop)

// 修饰键掩码
#define MOD_LCTRL   0x01
#define MOD_LSHIFT  0x02
#define MOD_LALT    0x04
#define MOD_LGUI    0x08
#define MOD_RCTRL   0x10
#define MOD_RSHIFT  0x20
#define MOD_RALT    0x40
#define MOD_RGUI    0x80

// 鼠标按键索引
#define MOUSE_BUTTON_LEFT    0
#define MOUSE_BUTTON_RIGHT   1
#define MOUSE_BUTTON_MIDDLE  2
#define MOUSE_BUTTON_X1      3
#define MOUSE_BUTTON_X2      4

#endif // IOCTL_DEFS_H
```

- [ ] **Step 2: 创建 autoxyq-error.h**

```c
#ifndef AUTOXYQ_ERROR_H
#define AUTOXYQ_ERROR_H

// 用户态 API 错误码
typedef enum {
    AUTOXYQ_OK                    =  0,
    AUTOXYQ_ERR_DRIVER_NOT_FOUND  = -1,
    AUTOXYQ_ERR_DEVICE_NOT_READY  = -2,
    AUTOXYQ_ERR_IOCTL_FAILED      = -3,
    AUTOXYQ_ERR_INVALID_PARAM     = -4,
    AUTOXYQ_ERR_QUEUE_FULL        = -5,
    AUTOXYQ_ERR_TIMEOUT           = -6,
} autoxyq_error_t;

// 获取错误码的可读描述
const char* autoxyq_strerror(int err);

#endif // AUTOXYQ_ERROR_H
```

- [ ] **Step 3: 创建 hid-report-desc.h**

```c
#ifndef HID_REPORT_DESC_H
#define HID_REPORT_DESC_H

#include <stdint.h>

// 键盘 HID Report 描述符 — 启动键盘 (Boot Keyboard)
static const uint8_t KEYBOARD_HID_REPORT_DESC[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    // 修饰键 (8 bit)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    // 保留位 (8 bit, fixed 0)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x03,       //   Input (Constant, Variable, Absolute)
    // 按键数组 (6 个按键)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x06,       //   Report Count (6)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data, Array, Absolute)
    0xC0,              // End Collection
};

// 键盘 HID Report 描述符长度
#define KEYBOARD_HID_REPORT_DESC_SIZE (sizeof(KEYBOARD_HID_REPORT_DESC))

// 键盘 Report 长度 (字节)
#define KEYBOARD_REPORT_SIZE 8

// 鼠标 HID Report 描述符 — 5 键 + 滚轮
static const uint8_t MOUSE_HID_REPORT_DESC[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    // 按键 (5 bit)
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (Button 1)
    0x29, 0x05,       //   Usage Maximum (Button 5)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x05,       //   Report Count (5)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    // 填充 (3 bit)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x03,       //   Report Count (3)
    0x81, 0x03,       //   Input (Constant, Variable, Absolute)
    // X 轴 (12 bit 相对)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x16, 0x01, 0xF8, //   Logical Minimum (-2047)
    0x26, 0xFF, 0x07, //   Logical Maximum (+2047)
    0x75, 0x0C,       //   Report Size (12)
    0x95, 0x02,       //   Report Count (2)
    0x81, 0x06,       //   Input (Data, Variable, Relative)
    // 滚轮 (8 bit)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x38,       //   Usage (Wheel)
    0x15, 0x81,       //   Logical Minimum (-127)
    0x25, 0x7F,       //   Logical Maximum (127)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x06,       //   Input (Data, Variable, Relative)
    0xC0,              // End Collection
};

// 鼠标 HID Report 描述符长度
#define MOUSE_HID_REPORT_DESC_SIZE (sizeof(MOUSE_HID_REPORT_DESC))

// 鼠标 Report 长度 (字节)
#define MOUSE_REPORT_SIZE 4

#endif // HID_REPORT_DESC_H
```

- [ ] **Step 4: 验证** — 确认头文件可被 C 编译器解析（后续 CI 编译即验证）

---

### Task 4: 驱动骨架

**Files:**
- Create: `G:\ai\autoxyq\driver\autoxyq-drv.h`
- Create: `G:\ai\autoxyq\driver\trace.h`
- Create: `G:\ai\autoxyq\driver\autoxyq-drv.c`

**Interfaces:**
- Consumes: `driver/ioctl-defs.h`, `driver/hid-report-desc.h` (Task 3)
- Produces:
  - `GetDriverModuleHandle()` — 驱动模块句柄访问器
  - 设备上下文结构 `DEVICE_CONTEXT`
  - `DriverEntry()` — UMDF 驱动入口
  - `EvtDriverDeviceAdd()` — 设备添加回调
  - `EvtDeviceD0Entry()` / `EvtDeviceD0Exit()` — 电源状态回调
  - `EvtDevicePrepareHardware()` / `EvtDeviceReleaseHardware()` — 硬件资源回调节桩

- [ ] **Step 1: 创建 autoxyq-drv.h**

```c
#ifndef AUTOXYQ_DRV_H
#define AUTOXYQ_DRV_H

#include <windows.h>
#include <wdf.h>

// 设备上下文 — 挂载在 WDFDEVICE 上
typedef struct _DEVICE_CONTEXT {
    // vhidmini 代理设备的接口引用 (后续任务填充)
    // 当前为骨架, 仅保留设备状态
    BOOLEAN IsPoweredOn;  // TRUE = 可接收 IOCTL, FALSE = 拒绝
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

// 获取设备上下文宏
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

// 获取此驱动模块句柄 (trace / 日志用)
HMODULE GetDriverModuleHandle(void);

// WDF 回调声明
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT EvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;

#endif // AUTOXYQ_DRV_H
```

- [ ] **Step 2: 创建 trace.h**

```c
#ifndef TRACE_H
#define TRACE_H

#include <windows.h>

// 轻量调试输出 — 使用 OutputDebugStringA, 可通过 DbgView 查看
// 驱动端不使用 printf (UMDF 运行在 WUDFHost.exe 中, 无控制台)

#ifdef _DEBUG
#define TRACE(fmt, ...) do { \
    char _buf[256]; \
    _snprintf_s(_buf, sizeof(_buf), _TRUNCATE, \
        "[autoxyq-drv] " fmt "\n", ##__VA_ARGS__); \
    OutputDebugStringA(_buf); \
} while(0)
#else
#define TRACE(fmt, ...) ((void)0)
#endif

#endif // TRACE_H
```

- [ ] **Step 3: 创建 autoxyq-drv.c**

```c
#include "autoxyq-drv.h"
#include "ioctl-defs.h"
#include "hid-report-desc.h"
#include "trace.h"

// 全局驱动模块句柄
static HMODULE g_DriverModuleHandle = NULL;

HMODULE GetDriverModuleHandle(void) {
    return g_DriverModuleHandle;
}

// 驱动入口点
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    (void)DriverObject;

    g_DriverModuleHandle = (HMODULE)DriverObject->DriverSection;

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDriverDeviceAdd);
    config.DriverPoolTag = 'qyxA'; // 'autoxyq' reverse — 内存池标签

    NTSTATUS status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        TRACE("WdfDriverCreate failed: 0x%08X", status);
    }

    return status;
}

// 设备添加 — 系统发现我们的设备时调用
NTSTATUS EvtDriverDeviceAdd(
    _In_ WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    (void)Driver;

    TRACE("EvtDriverDeviceAdd called");

    // 配置设备为 UMDF 用户态驱动
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);

    // 设备上下文
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    // 电源回调
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDeviceD0Entry = EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = EvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDFDEVICE device;
    NTSTATUS status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        TRACE("WdfDeviceCreate failed: 0x%08X", status);
        return status;
    }

    // 初始化设备上下文
    PDEVICE_CONTEXT ctx = GetDeviceContext(device);
    ctx->IsPoweredOn = FALSE;

    // 创建默认 IOCTL 队列 (将在 Task 7 完善)
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = NULL; // Task 7 中替换为 EvtIoDeviceControl

    WDFQUEUE queue;
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (!NT_SUCCESS(status)) {
        TRACE("WdfIoQueueCreate failed: 0x%08X", status);
        return status;
    }

    // 创建设备接口 (用户态通过此 GUID 发现设备)
    status = WdfDeviceCreateDeviceInterface(
        device,
        &GUID_AUTOXYQ_DEVICE_INTERFACE,
        NULL);
    if (!NT_SUCCESS(status)) {
        TRACE("WdfDeviceCreateDeviceInterface failed: 0x%08X", status);
        return status;
    }

    TRACE("Device created successfully");
    return STATUS_SUCCESS;
}

// 设备上电
NTSTATUS EvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    (void)PreviousState;
    PDEVICE_CONTEXT ctx = GetDeviceContext(Device);
    ctx->IsPoweredOn = TRUE;
    TRACE("Device powered on (D0)");
    return STATUS_SUCCESS;
}

// 设备下电
NTSTATUS EvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    (void)TargetState;
    PDEVICE_CONTEXT ctx = GetDeviceContext(Device);
    ctx->IsPoweredOn = FALSE;
    TRACE("Device powered off");
    return STATUS_SUCCESS;
}

// 硬件准备 (存根, vhidmini 代理在后续任务完善)
NTSTATUS EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)Device;
    (void)ResourcesRaw;
    (void)ResourcesTranslated;
    TRACE("EvtDevicePrepareHardware");
    return STATUS_SUCCESS;
}

// 硬件释放 (存根)
NTSTATUS EvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)Device;
    (void)ResourcesTranslated;
    TRACE("EvtDeviceReleaseHardware");
    return STATUS_SUCCESS;
}
```

- [ ] **Step 4: 验证** — 后续 CI 编译通过即验证

---

### Task 5: 虚拟键盘 HID 模块

**Files:**
- Create: `G:\ai\autoxyq\driver\hid-keyboard.c`

**Interfaces:**
- Consumes: `driver/hid-report-desc.h` (Task 3), `driver/autoxyq-drv.h` (Task 4)
- Produces:
  - `HidKeyboard_Init(HANDLE vhidHandle)` — 初始化键盘 HID 设备
  - `HidKeyboard_SubmitReport(HANDLE vhidHandle, const keyboard_report_t* report)` — 提交键盘 Report
  - `HidKeyboard_Reset(HANDLE vhidHandle)` — 重置 (全零 Report)

- [ ] **Step 1: 创建 hid-keyboard.c**

```c
#include <windows.h>
#include <stdint.h>
#include "hid-report-desc.h"

// 键盘状态跟踪
typedef struct {
    uint8_t modifier_bitmap;
    uint8_t key_codes[6];
} keyboard_state_t;

static keyboard_state_t g_KeyboardState;

// 初始化键盘状态
void HidKeyboard_Init(void) {
    g_KeyboardState.modifier_bitmap = 0;
    for (int i = 0; i < 6; i++) {
        g_KeyboardState.key_codes[i] = 0;
    }
}

// 构造 HID Report 并发送到 vhidmini
// vhidHandle: vhidmini 提供的设备句柄 (在 Task 7 中传入)
BOOL HidKeyboard_SubmitReport(HANDLE vhidHandle, const keyboard_report_t* report) {
    if (report == NULL) {
        return FALSE;
    }

    // 通过 vhidmini 接口发送 HID Report
    // vhidmini 使用 IOCTL_VHIDMINI_SUBMIT_REPORT (后续任务中定义)
    // 当前存根，等待 vhidmini 集成
    (void)vhidHandle;

    // 更新内部状态
    g_KeyboardState.modifier_bitmap = report->modifier_bitmap;
    for (int i = 0; i < 6; i++) {
        g_KeyboardState.key_codes[i] = report->key_codes[i];
    }

    return TRUE;
}

// 发送全零 Report (所有按键弹起)
BOOL HidKeyboard_Reset(HANDLE vhidHandle) {
    keyboard_report_t resetReport;
    resetReport.modifier_bitmap = 0;
    resetReport.reserved = 0;
    for (int i = 0; i < 6; i++) {
        resetReport.key_codes[i] = 0;
    }
    return HidKeyboard_SubmitReport(vhidHandle, &resetReport);
}

// 获取当前键盘状态 (只读)
const keyboard_state_t* HidKeyboard_GetState(void) {
    return &g_KeyboardState;
}
```

---

### Task 6: 虚拟鼠标 HID 模块

**Files:**
- Create: `G:\ai\autoxyq\driver\hid-mouse.c`

**Interfaces:**
- Consumes: `driver/hid-report-desc.h` (Task 3)
- Produces:
  - `HidMouse_Init(void)` — 初始化鼠标状态
  - `HidMouse_SubmitReport(HANDLE vhidHandle, const mouse_report_t* report)` — 提交鼠标 Report
  - `HidMouse_Reset(HANDLE vhidHandle)` — 重置 (全零 Report)

- [ ] **Step 1: 创建 hid-mouse.c**

```c
#include <windows.h>
#include <stdint.h>
#include "hid-report-desc.h"

// 鼠标状态跟踪
typedef struct {
    uint8_t  button_mask;
    int16_t  dx;
    int16_t  dy;
    int8_t   wheel;
} mouse_state_t;

static mouse_state_t g_MouseState;

// 参数范围校验
static BOOL MouseReport_Validate(const mouse_report_t* report) {
    if (report == NULL) {
        return FALSE;
    }
    // 按键位图仅低 5 位有效
    if (report->button_mask > 0x1F) {
        return FALSE;
    }
    // 位移范围
    if (report->dx < -2047 || report->dx > 2047) {
        return FALSE;
    }
    if (report->dy < -2047 || report->dy > 2047) {
        return FALSE;
    }
    return TRUE;
}

void HidMouse_Init(void) {
    g_MouseState.button_mask = 0;
    g_MouseState.dx = 0;
    g_MouseState.dy = 0;
    g_MouseState.wheel = 0;
}

BOOL HidMouse_SubmitReport(HANDLE vhidHandle, const mouse_report_t* report) {
    if (!MouseReport_Validate(report)) {
        return FALSE;
    }

    // 通过 vhidmini 接口发送 HID Report
    (void)vhidHandle;

    // 更新内部状态
    g_MouseState.button_mask = report->button_mask;
    g_MouseState.dx = report->dx;
    g_MouseState.dy = report->dy;
    g_MouseState.wheel = report->wheel;

    return TRUE;
}

BOOL HidMouse_Reset(HANDLE vhidHandle) {
    mouse_report_t resetReport;
    resetReport.button_mask = 0;
    resetReport.dx = 0;
    resetReport.dy = 0;
    resetReport.wheel = 0;
    return HidMouse_SubmitReport(vhidHandle, &resetReport);
}

const mouse_state_t* HidMouse_GetState(void) {
    return &g_MouseState;
}
```

---

### Task 7: IOCTL 请求处理器

**Files:**
- Create: `G:\ai\autoxyq\driver\ioctl-handler.c`
- Modify: `G:\ai\autoxyq\driver\autoxyq-drv.c` (注册 EvtIoDeviceControl 回调, 更新设备上下文)

**Interfaces:**
- Consumes: `driver/ioctl-defs.h` (Task 3), `driver/autoxyq-drv.h` (Task 4), `driver/hid-keyboard.c` (Task 5), `driver/hid-mouse.c` (Task 6)
- Produces:
  - `EvtIoDeviceControl()` — IOCTL 请求分发器
  - 更新 `DEVICE_CONTEXT` 增加 vhidmini 句柄字段

- [ ] **Step 1: 创建 ioctl-handler.c**

```c
#include <windows.h>
#include <wdf.h>
#include "autoxyq-drv.h"
#include "ioctl-defs.h"
#include "trace.h"

// 前向声明 (来自 hid-keyboard.c 和 hid-mouse.c)
extern void HidKeyboard_Init(void);
extern BOOL HidKeyboard_SubmitReport(HANDLE vhidHandle, const keyboard_report_t* report);
extern BOOL HidKeyboard_Reset(HANDLE vhidHandle);

extern void HidMouse_Init(void);
extern BOOL HidMouse_SubmitReport(HANDLE vhidHandle, const mouse_report_t* report);
extern BOOL HidMouse_Reset(HANDLE vhidHandle);

// IOCTL 请求分发器
VOID EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode)
{
    (void)OutputBufferLength;

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT ctx = GetDeviceContext(device);

    // 设备未就绪则拒绝
    if (!ctx->IsPoweredOn) {
        WdfRequestComplete(Request, STATUS_DEVICE_NOT_READY);
        return;
    }

    switch (IoControlCode) {

    case IOCTL_KEYBOARD_REPORT: {
        if (InputBufferLength < sizeof(keyboard_report_t)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        WDFMEMORY inputMemory;
        status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
        if (!NT_SUCCESS(status)) {
            break;
        }

        keyboard_report_t* report = (keyboard_report_t*)WdfMemoryGetBuffer(
            inputMemory, NULL);

        // 参数校验: reserved 字段必须为 0
        if (report->reserved != 0) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // 参数校验: 修饰键位图不超出范围
        // (无限制 — 8 位任意组合均合法)

        // 参数校验: key_codes 中的值必须在 0-0x65 范围内或全 0
        for (int i = 0; i < 6; i++) {
            if (report->key_codes[i] > 0x65) {
                status = STATUS_INVALID_PARAMETER;
                TRACE("Invalid key code: 0x%02X at index %d", report->key_codes[i], i);
                break;
            }
        }
        if (!NT_SUCCESS(status)) break;

        // 通过 vhidmini 发送
        if (!HidKeyboard_SubmitReport(ctx->VhidHandle, report)) {
            status = STATUS_UNSUCCESSFUL;
        }
        break;
    }

    case IOCTL_MOUSE_REPORT: {
        if (InputBufferLength < sizeof(mouse_report_t)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        WDFMEMORY inputMemory;
        status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
        if (!NT_SUCCESS(status)) {
            break;
        }

        mouse_report_t* report = (mouse_report_t*)WdfMemoryGetBuffer(
            inputMemory, NULL);

        // 参数校验: button_mask 仅低 5 位有效
        if (report->button_mask > 0x1F) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // 参数校验: 位移范围
        if (report->dx < -2047 || report->dx > 2047) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (report->dy < -2047 || report->dy > 2047) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!HidMouse_SubmitReport(ctx->VhidHandle, report)) {
            status = STATUS_UNSUCCESSFUL;
        }
        break;
    }

    case IOCTL_RESET_DEVICES: {
        TRACE("Resetting all devices");
        HidKeyboard_Reset(ctx->VhidHandle);
        HidMouse_Reset(ctx->VhidHandle);
        status = STATUS_SUCCESS;
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestComplete(Request, status);
}
```

- [ ] **Step 2: 更新 DEVICE_CONTEXT 增加 VhidHandle**

修改 `driver/autoxyq-drv.h` 中的 DEVICE_CONTEXT:

```c
typedef struct _DEVICE_CONTEXT {
    BOOLEAN IsPoweredOn;
    HANDLE  VhidHandle;   // vhidmini 设备句柄 (用于提交 HID Report)
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;
```

- [ ] **Step 3: 在 autoxyq-drv.c 中注册 IOCTL 回调 + 初始化 vhid/HID 模块**

修改 `driver/autoxyq-drv.c` 的 `EvtDriverDeviceAdd`:
- 将 `queueConfig.EvtIoDeviceControl = NULL;` 替换为 `queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;`
- 在设备创建后添加 `HidKeyboard_Init()` 和 `HidMouse_Init()` 调用
- 在 `EvtDevicePrepareHardware` 中打开 vhidmini 设备, 存入 `ctx->VhidHandle`
- 在 `EvtDeviceReleaseHardware` 中关闭 vhidmini 句柄

添加 extern 声明:
```c
extern VOID EvtIoDeviceControl(_In_ WDFQUEUE, _In_ WDFREQUEST, _In_ size_t, _In_ size_t, _In_ ULONG);
extern void HidKeyboard_Init(void);
extern void HidMouse_Init(void);
```

在 EvtDevicePrepareHardware 中添加 vhidmini 设备打开逻辑:
```c
NTSTATUS EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)ResourcesRaw;
    (void)ResourcesTranslated;

    PDEVICE_CONTEXT ctx = GetDeviceContext(Device);

    // 打开 vhidmini 设备 (接口 GUID 来自 vhidmini 驱动)
    // 注意: 实际 GUID 需与 vhidmini 示例中的定义匹配
    // 此处使用占位逻辑，具体集成在 Task 8 INF 安装时连接
    ctx->VhidHandle = INVALID_HANDLE_VALUE;
    TRACE("EvtDevicePrepareHardware - vhid handle placeholder");

    return STATUS_SUCCESS;
}

NTSTATUS EvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)ResourcesTranslated;

    PDEVICE_CONTEXT ctx = GetDeviceContext(Device);
    if (ctx->VhidHandle != INVALID_HANDLE_VALUE && ctx->VhidHandle != NULL) {
        CloseHandle(ctx->VhidHandle);
        ctx->VhidHandle = INVALID_HANDLE_VALUE;
    }

    TRACE("EvtDeviceReleaseHardware");
    return STATUS_SUCCESS;
}
```

---

### Task 8: 驱动 INF 与 CMakeLists

**Files:**
- Create: `G:\ai\autoxyq\driver\autoxyq-drv.inx`
- Create: `G:\ai\autoxyq\driver\CMakeLists.txt`

**Interfaces:**
- Consumes: 所有驱动文件 (Task 3-7)
- Produces: INF 安装文件, 驱动编译目标

- [ ] **Step 1: 创建 autoxyq-drv.inx**

```ini
; autoxyq-drv.inx - UMDF Virtual HID Driver Installation File
; 基于 vhidmini 示例模板

[Version]
Signature   = "$Windows NT$"
Class       = HIDClass
ClassGuid   = {745a17a0-74d3-11d0-b6fe-00a0c90f57da}
Provider    = %ManufacturerName%
CatalogFile = autoxyq-drv.cat
DriverVer   = 08/06/2026,1.0.0.0
PnpLockdown = 1

[SourceDisksNames]
1 = %DiskName%

[SourceDisksFiles]
autoxyq-drv.dll = 1
WdfCoInstaller01011.dll = 1

[DestinationDirs]
CopyFunctionDriver = 12          ; %windir%\system32\drivers\umdf
CopyCoInstaller    = 11          ; %windir%\system32

[Manufacturer]
%ManufacturerName% = Standard,NTamd64

[Standard.NTamd64]
%DeviceName% = autoxyq_install, ACPI\AUTOXYQ

; ===== 安装节 =====
[autoxyq_install.NT]
CopyFiles = CopyFunctionDriver, CopyCoInstaller

[autoxyq_install.NT.Services]
AddService = WUDFRd, 0x000001fa, WUDFRd_ServiceInstall
AddService = autoxyq-drv,, AutoxyqDrv_ServiceInstall

[autoxyq_install.NT.Wdf]
UmdfService = autoxyq-drv, AutoxyqDrv_UmdfService
UmdfServiceOrder = autoxyq-drv

; ===== 文件拷贝 =====
[CopyFunctionDriver]
autoxyq-drv.dll

[CopyCoInstaller]
WdfCoInstaller01011.dll

; ===== 服务配置 =====
[WUDFRd_ServiceInstall]
ServiceType    = 1
StartType      = 3
ErrorControl   = 1
ServiceBinary  = %12%\WUDFRd.sys

[AutoxyqDrv_ServiceInstall]
ServiceType    = 1
StartType      = 3
ErrorControl   = 1
ServiceBinary  = %12%\autoxyq-drv.dll

[AutoxyqDrv_UmdfService]
UmdfLibraryVersion   = 2.0
ServiceLibrary       = autoxyq-drv.dll

; ===== 字符串 =====
[Strings]
ManufacturerName = "autoxyq"
DeviceName       = "autoxyq Virtual HID Device"
DiskName         = "autoxyq Driver Installation Disk"
```

- [ ] **Step 2: 创建 driver/CMakeLists.txt**

```cmake
# 驱动源文件
set(DRIVER_SOURCES
    autoxyq-drv.c
    hid-keyboard.c
    hid-mouse.c
    ioctl-handler.c
)

# UMDF 驱动编译为 DLL
add_library(autoxyq-drv SHARED ${DRIVER_SOURCES})

# 设置 UMDF 驱动属性
set_target_properties(autoxyq-drv PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
)

# WDK include 路径由 setup-wdk action 自动配置
# 本地需确保 WDK 环境变量已设置

# 链接 WDF 用户态库
target_link_libraries(autoxyq-drv
    WdfUmDriverStub.lib
)

# 驱动专用编译定义
target_compile_definitions(autoxyq-drv PRIVATE
    _UMDF_
    UMDF2_USE_VERSION=2
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
```

- [ ] **Step 3: 更新根 CMakeLists.txt** — 取消 `add_subdirectory(driver)` 注释

---

### Task 9: 用户态库 API 头文件

**Files:**
- Create: `G:\ai\autoxyq\lib\autoxyq.h`

**Interfaces:**
- Consumes: `lib/autoxyq-error.h` (Task 3), `driver/ioctl-defs.h` (Task 3 — 复用 device GUID)
- Produces: 完整公开 API 声明

- [ ] **Step 1: 创建 autoxyq.h**

```c
#ifndef AUTOXYQ_H
#define AUTOXYQ_H

#include <stdint.h>
#include "autoxyq-error.h"

// 轨迹类型枚举
typedef enum {
    TRAJECTORY_LINEAR  = 0,
    TRAJECTORY_BEZIER2 = 1,
    TRAJECTORY_BEZIER3 = 2,
} trajectory_type_t;

// ---- 生命周期 ----

/**
 * 初始化 autoxyq 库，发现并打开驱动设备。
 * 返回 AUTOXYQ_OK 成功，否则返回错误码。
 */
int autoxyq_init(void);

/**
 * 关闭 autoxyq 库。
 * 自动清空动作队列、发送重置命令、关闭驱动句柄。
 */
void autoxyq_shutdown(void);

// ---- 键盘操作 ----

/** 按下按键 (USB HID Usage ID) */
int autoxyq_key_down(uint8_t usb_usage_id);

/** 弹起按键 (USB HID Usage ID) */
int autoxyq_key_up(uint8_t usb_usage_id);

/** 按下 + 延时 + 弹起 */
int autoxyq_key_press(uint8_t usb_usage_id, uint32_t duration_ms);

// ---- 鼠标操作 (相对位移) ----

/** 瞬间相对移动 */
int autoxyq_mouse_move(int16_t dx, int16_t dy);

/** 带轨迹的平滑相对移动 */
int autoxyq_mouse_move_ex(int16_t dx, int16_t dy, uint32_t duration_ms,
                           trajectory_type_t type);

// ---- 鼠标操作 (绝对坐标) ----

/**
 * 移动到屏幕绝对坐标。
 * 通过持续发送相对位移模拟，使用轨迹插值。
 * x, y: 目标屏幕坐标 (像素)
 * duration_ms: 移动总时长 (毫秒)
 * type: 轨迹曲线类型
 */
int autoxyq_mouse_move_to(int x, int y, uint32_t duration_ms,
                           trajectory_type_t type);

// ---- 鼠标按键 ----

/** 按下鼠标按钮 (1=左, 2=右, 3=中, 4=侧键1, 5=侧键2) */
int autoxyq_mouse_button_down(uint8_t button);

/** 弹起鼠标按钮 */
int autoxyq_mouse_button_up(uint8_t button);

/** 滚轮滚动 (正值向上, 负值向下) */
int autoxyq_mouse_scroll(int8_t delta);

// ---- 配置 ----

/** 设置全局随机延迟范围 (毫秒)，min_ms <= max_ms */
void autoxyq_set_delay_range(uint32_t min_ms, uint32_t max_ms);

/** 设置默认轨迹类型 */
void autoxyq_set_trajectory_type(trajectory_type_t type);

/** 设置微抖动幅度 (像素), 0 关闭 */
void autoxyq_set_jitter(uint8_t amplitude_px);

// ---- 异步控制 ----

/** 阻塞直到动作队列清空 */
void autoxyq_flush(void);

#endif // AUTOXYQ_H
```

---

### Task 10: 用户态库入口 + IOCTL 客户端

**Files:**
- Create: `G:\ai\autoxyq\lib\ioctl-client.c`
- Create: `G:\ai\autoxyq\lib\autoxyq.c`

**Interfaces:**
- Consumes: `lib/autoxyq.h` (Task 9), `lib/autoxyq-error.h` (Task 3), `driver/ioctl-defs.h` (Task 3)
- Produces:
  - `autoxyq_init()` / `autoxyq_shutdown()` 实现
  - IOCTL 通信内部函数 (`ioctl_send_keyboard`, `ioctl_send_mouse`, `ioctl_reset_devices`)
  - 全局驱动句柄 `g_DeviceHandle`
  - `autoxyq_strerror()` 实现

- [ ] **Step 1: 创建 ioctl-client.c**

```c
#include <windows.h>
#include <setupapi.h>
#include <stdint.h>
#include "ioctl-defs.h"
#include "autoxyq-error.h"

// test mode: 如果无法找到设备 GUID, 链接 device interface 发现
// 注意: ioctl-defs.h 中的 GUID_AUTOXYQ_DEVICE_INTERFACE 使用 DEFINE_GUID
// 在 .c 中包含 initguid.h (已在 ioctl-defs.h 中包含)

// 发现并打开驱动设备
// 返回设备句柄; 失败返回 INVALID_HANDLE_VALUE
HANDLE ioctl_open_device(void) {
    // 获取设备接口列表
    SP_DEVICE_INTERFACE_DATA ifData;
    ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    HDEVINFO devInfo = SetupDiGetClassDevsW(
        &GUID_AUTOXYQ_DEVICE_INTERFACE,
        NULL, NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (devInfo == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    // 枚举第一个匹配设备
    if (!SetupDiEnumDeviceInterfaces(devInfo, NULL,
            &GUID_AUTOXYQ_DEVICE_INTERFACE, 0, &ifData)) {
        SetupDiDestroyDeviceInfoList(devInfo);
        return INVALID_HANDLE_VALUE;
    }

    // 获取设备路径长度
    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, NULL, 0, &requiredSize, NULL);

    // 获取设备路径
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);
    if (detailData == NULL) {
        SetupDiDestroyDeviceInfoList(devInfo);
        return INVALID_HANDLE_VALUE;
    }
    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData,
            detailData, requiredSize, NULL, NULL)) {
        free(detailData);
        SetupDiDestroyDeviceInfoList(devInfo);
        return INVALID_HANDLE_VALUE;
    }

    // 打开设备
    HANDLE hDevice = CreateFileW(
        detailData->DevicePath,
        GENERIC_WRITE,
        0,                    // 独占访问
        NULL,                 // 默认安全属性
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    free(detailData);
    SetupDiDestroyDeviceInfoList(devInfo);

    return hDevice;
}

// 关闭设备
void ioctl_close_device(HANDLE hDevice) {
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
}

// 发送键盘 Report
int ioctl_send_keyboard(HANDLE hDevice, const keyboard_report_t* report) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }
    if (report == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_KEYBOARD_REPORT,
        (LPVOID)report, sizeof(keyboard_report_t),
        NULL, 0,
        &bytesReturned,
        NULL);

    return ok ? AUTOXYQ_OK : AUTOXYQ_ERR_IOCTL_FAILED;
}

// 发送鼠标 Report
int ioctl_send_mouse(HANDLE hDevice, const mouse_report_t* report) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }
    if (report == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_MOUSE_REPORT,
        (LPVOID)report, sizeof(mouse_report_t),
        NULL, 0,
        &bytesReturned,
        NULL);

    return ok ? AUTOXYQ_OK : AUTOXYQ_ERR_IOCTL_FAILED;
}

// 重置设备
int ioctl_reset_devices(HANDLE hDevice) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_RESET_DEVICES,
        NULL, 0,
        NULL, 0,
        &bytesReturned,
        NULL);

    return ok ? AUTOXYQ_OK : AUTOXYQ_ERR_IOCTL_FAILED;
}
```

- [ ] **Step 2: 创建 autoxyq.c (库入口 + 基础键盘 API)**

```c
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include "autoxyq.h"
#include "ioctl-defs.h"

// ioctl-client.c 声明
extern HANDLE ioctl_open_device(void);
extern void ioctl_close_device(HANDLE hDevice);
extern int ioctl_send_keyboard(HANDLE hDevice, const keyboard_report_t* report);
extern int ioctl_send_mouse(HANDLE hDevice, const mouse_report_t* report);
extern int ioctl_reset_devices(HANDLE hDevice);

// 轨迹引擎声明 (Task 12 实现)
extern int trajectory_generate_path(int x0, int y0, int x1, int y1,
    uint32_t duration_ms, trajectory_type_t type,
    int16_t** out_path, uint32_t* out_count);

// 随机延迟引擎声明 (Task 11 实现)
extern uint32_t delay_random_ms(void);
extern void delay_set_range(uint32_t min_ms, uint32_t max_ms);
extern void delay_sleep_us(uint64_t us);

// 全局状态
static HANDLE g_DeviceHandle = INVALID_HANDLE_VALUE;
static uint32_t g_DelayMinMs = 10;
static uint32_t g_DelayMaxMs = 50;
static trajectory_type_t g_DefaultTrajectory = TRAJECTORY_BEZIER3;
static uint8_t g_JitterAmplitude = 2;  // 默认 2px 微抖动
static uint8_t g_MouseButtonState = 0; // 当前鼠标按键状态位图
static keyboard_report_t g_KeyboardState; // 当前键盘状态
static uint32_t g_RandomSeed = 0;

// ---- 生命周期 ----

int autoxyq_init(void) {
    if (g_DeviceHandle != INVALID_HANDLE_VALUE) {
        return AUTOXYQ_OK; // 已初始化
    }

    g_DeviceHandle = ioctl_open_device();
    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DRIVER_NOT_FOUND;
    }

    // 初始化随机种子
    g_RandomSeed = (uint32_t)GetTickCount64();

    // 初始化键盘状态
    g_KeyboardState.modifier_bitmap = 0;
    g_KeyboardState.reserved = 0;
    for (int i = 0; i < 6; i++) {
        g_KeyboardState.key_codes[i] = 0;
    }

    // 重置设备状态
    ioctl_reset_devices(g_DeviceHandle);

    return AUTOXYQ_OK;
}

void autoxyq_shutdown(void) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    // 重置所有设备
    ioctl_reset_devices(g_DeviceHandle);

    // 关闭设备
    ioctl_close_device(g_DeviceHandle);
    g_DeviceHandle = INVALID_HANDLE_VALUE;
}

// ---- 键盘操作 ----

static int find_key_slot(const uint8_t* keys, uint8_t target) {
    for (int i = 0; i < 6; i++) {
        if (keys[i] == target) return i;
    }
    return -1;
}

static int find_empty_slot(const uint8_t* keys) {
    return find_key_slot(keys, 0);
}

int autoxyq_key_down(uint8_t usb_usage_id) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }
    if (usb_usage_id == 0 || usb_usage_id > 0x65) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    // 检查是否已在按下状态
    if (find_key_slot(g_KeyboardState.key_codes, usb_usage_id) >= 0) {
        return AUTOXYQ_OK; // 已按下，幂等
    }

    // 找到空槽位
    int slot = find_empty_slot(g_KeyboardState.key_codes);
    if (slot < 0) {
        return AUTOXYQ_ERR_QUEUE_FULL; // 6 键已满
    }

    g_KeyboardState.key_codes[slot] = usb_usage_id;
    return ioctl_send_keyboard(g_DeviceHandle, &g_KeyboardState);
}

int autoxyq_key_up(uint8_t usb_usage_id) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }
    if (usb_usage_id == 0 || usb_usage_id > 0x65) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    int slot = find_key_slot(g_KeyboardState.key_codes, usb_usage_id);
    if (slot < 0) {
        return AUTOXYQ_OK; // 未按下，幂等
    }

    g_KeyboardState.key_codes[slot] = 0;
    return ioctl_send_keyboard(g_DeviceHandle, &g_KeyboardState);
}

int autoxyq_key_press(uint8_t usb_usage_id, uint32_t duration_ms) {
    int ret = autoxyq_key_down(usb_usage_id);
    if (ret != AUTOXYQ_OK) return ret;

    if (duration_ms > 0) {
        delay_sleep_us((uint64_t)duration_ms * 1000);
    }

    return autoxyq_key_up(usb_usage_id);
}
```

- [ ] **Step 3: 继续 autoxyq.c — 鼠标操作基础实现**

```c
// ---- 鼠标操作 ----

static int send_mouse_report(int16_t dx, int16_t dy, int8_t wheel) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        return AUTOXYQ_ERR_DEVICE_NOT_READY;
    }

    mouse_report_t report;
    report.button_mask = g_MouseButtonState;
    report.dx = dx;
    report.dy = dy;
    report.wheel = wheel;

    return ioctl_send_mouse(g_DeviceHandle, &report);
}

int autoxyq_mouse_move(int16_t dx, int16_t dy) {
    return send_mouse_report(dx, dy, 0);
}

int autoxyq_mouse_move_ex(int16_t dx, int16_t dy, uint32_t duration_ms,
                           trajectory_type_t type) {
    if (duration_ms == 0) {
        return autoxyq_mouse_move(dx, dy);
    }

    int16_t* path = NULL;
    uint32_t count = 0;
    int ret = trajectory_generate_path(0, 0, dx, dy, duration_ms, type,
                                        &path, &count);
    if (ret != AUTOXYQ_OK) return ret;

    // 逐帧发送
    for (uint32_t i = 0; i < count; i++) {
        uint32_t before_ms = (uint32_t)(GetTickCount64() & 0xFFFFFFFF);

        ret = send_mouse_report(path[i * 2], path[i * 2 + 1], 0);
        if (ret != AUTOXYQ_OK) {
            free(path);
            return ret;
        }

        // 帧间延迟
        uint32_t elapsed_ms = (uint32_t)(GetTickCount64() & 0xFFFFFFFF) - before_ms;
        uint32_t frame_delay = delay_random_ms();
        if (frame_delay > elapsed_ms) {
            delay_sleep_us((uint64_t)(frame_delay - elapsed_ms) * 1000);
        }
    }

    free(path);
    return AUTOXYQ_OK;
}

int autoxyq_mouse_move_to(int x, int y, uint32_t duration_ms,
                           trajectory_type_t type) {
    // 获取当前鼠标位置作为起点
    POINT currentPos;
    if (!GetCursorPos(&currentPos)) {
        return AUTOXYQ_ERR_IOCTL_FAILED;
    }

    int16_t dx = (int16_t)(x - currentPos.x);
    int16_t dy = (int16_t)(y - currentPos.y);

    return autoxyq_mouse_move_ex(dx, dy, duration_ms, type);
}

int autoxyq_mouse_button_down(uint8_t button) {
    if (button < 1 || button > 5) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    g_MouseButtonState |= (1 << (button - 1));
    return send_mouse_report(0, 0, 0);
}

int autoxyq_mouse_button_up(uint8_t button) {
    if (button < 1 || button > 5) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    g_MouseButtonState &= ~(1 << (button - 1));
    return send_mouse_report(0, 0, 0);
}

int autoxyq_mouse_scroll(int8_t delta) {
    return send_mouse_report(0, 0, delta);
}

// ---- 配置 ----

void autoxyq_set_delay_range(uint32_t min_ms, uint32_t max_ms) {
    if (min_ms > max_ms) {
        uint32_t tmp = min_ms;
        min_ms = max_ms;
        max_ms = tmp;
    }
    g_DelayMinMs = min_ms;
    g_DelayMaxMs = max_ms;
    delay_set_range(min_ms, max_ms);
}

void autoxyq_set_trajectory_type(trajectory_type_t type) {
    if (type <= TRAJECTORY_BEZIER3) {
        g_DefaultTrajectory = type;
    }
}

void autoxyq_set_jitter(uint8_t amplitude_px) {
    g_JitterAmplitude = amplitude_px;
}

// ---- 异步控制 ----

void autoxyq_flush(void) {
    // 当前为同步模式，队列在 Task 13 中实现后扩展
    // flush 在同步模式下为空操作
}

// ---- 错误处理 ----

const char* autoxyq_strerror(int err) {
    switch (err) {
        case AUTOXYQ_OK:                    return "Success";
        case AUTOXYQ_ERR_DRIVER_NOT_FOUND:  return "Driver not found. Install the driver first.";
        case AUTOXYQ_ERR_DEVICE_NOT_READY:  return "Device not ready. Driver loaded but device not powered.";
        case AUTOXYQ_ERR_IOCTL_FAILED:      return "IOCTL communication failed.";
        case AUTOXYQ_ERR_INVALID_PARAM:     return "Invalid parameter.";
        case AUTOXYQ_ERR_QUEUE_FULL:        return "Action queue full or keyboard buffer full.";
        case AUTOXYQ_ERR_TIMEOUT:           return "Operation timed out.";
        default:                            return "Unknown error.";
    }
}
```

---

### Task 11: 随机延迟引擎

**Files:**
- Create: `G:\ai\autoxyq\lib\delay.h`
- Create: `G:\ai\autoxyq\lib\delay.c`

**Interfaces:**
- Consumes: (无依赖, 纯算法模块)
- Produces:
  - `delay_set_range(uint32_t min_ms, uint32_t max_ms)` — 配置延迟范围
  - `delay_random_ms(void)` — 返回 [min, max] 范围内随机毫秒值
  - `delay_sleep_us(uint64_t us)` — 微秒级高精度睡眠

- [ ] **Step 1: 创建 delay.h**

```c
#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

// 设置随机延迟范围 (毫秒)
void delay_set_range(uint32_t min_ms, uint32_t max_ms);

// 返回 [min_ms, max_ms] 范围内均匀随机值 (毫秒)
uint32_t delay_random_ms(void);

// 微秒级高精度睡眠
void delay_sleep_us(uint64_t us);

#endif // DELAY_H
```

- [ ] **Step 2: 创建 delay.c**

```c
#include <windows.h>
#include <stdlib.h>
#include "delay.h"

static uint32_t g_DelayMinMs = 10;
static uint32_t g_DelayMaxMs = 50;

void delay_set_range(uint32_t min_ms, uint32_t max_ms) {
    g_DelayMinMs = min_ms;
    g_DelayMaxMs = max_ms;
}

uint32_t delay_random_ms(void) {
    if (g_DelayMinMs >= g_DelayMaxMs) {
        return g_DelayMinMs;
    }

    // 使用 rand() 的均匀分布
    uint32_t range = g_DelayMaxMs - g_DelayMinMs;
    uint32_t random_offset = (uint32_t)rand() % (range + 1);
    return g_DelayMinMs + random_offset;
}

void delay_sleep_us(uint64_t us) {
    if (us == 0) return;

    // 使用 QueryPerformanceCounter 实现高精度忙等待
    // 对于亚毫秒级延迟使用自旋，毫秒级以上使用 Sleep
    if (us >= 1000) {
        // 毫秒级部分使用 Sleep
        DWORD ms = (DWORD)(us / 1000);
        Sleep(ms);
        us = us % 1000;
    }

    if (us == 0) return;

    // 微秒级部分使用性能计数器自旋
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        // 回退到 Sleep(1)
        Sleep(1);
        return;
    }

    LARGE_INTEGER start, current;
    QueryPerformanceCounter(&start);

    // 计算目标计数值: us * freq / 1_000_000
    uint64_t target_ticks = (us * freq.QuadPart) / 1000000ULL;

    do {
        QueryPerformanceCounter(&current);
    } while ((uint64_t)(current.QuadPart - start.QuadPart) < target_ticks);
}
```

---

### Task 12: 轨迹插值引擎

**Files:**
- Create: `G:\ai\autoxyq\lib\trajectory.h`
- Create: `G:\ai\autoxyq\lib\trajectory.c`

**Interfaces:**
- Consumes: `lib/autoxyq.h` (trajectory_type_t 定义 — Task 9), `lib/autoxyq-error.h` (Task 3)
- Produces:
  - `trajectory_generate_path(x0, y0, x1, y1, duration_ms, type, out_path, out_count)` — 生成路径点数组

- [ ] **Step 1: 创建 trajectory.h**

```c
#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <stdint.h>
#include "autoxyq.h"
#include "autoxyq-error.h"

// 生成路径点数组
// x0, y0: 起点 (相对坐标)
// x1, y1: 终点 (相对坐标)
// duration_ms: 移动总时长
// type: 轨迹类型
// out_path: 输出路径点数组 (调用方负责 free), 格式 [x0, y0, x1, y1, ...]
// out_count: 输出路径点数量
// 返回 AUTOXYQ_OK 成功
int trajectory_generate_path(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              uint32_t duration_ms,
                              trajectory_type_t type,
                              int16_t** out_path,
                              uint32_t* out_count);

#endif // TRAJECTORY_H
```

- [ ] **Step 2: 创建 trajectory.c**

```c
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "trajectory.h"
#include "delay.h"

// 外部变量 — 来自 autoxyq.c 或全局配置
extern uint8_t g_JitterAmplitude;  // 微抖动幅度
extern uint32_t g_RandomSeed;

// 三点线性插值
static double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// 二次贝塞尔: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
static double bezier2(double p0, double p1, double p2, double t) {
    double u = 1.0 - t;
    return u * u * p0 + 2.0 * u * t * p1 + t * t * p2;
}

// 三次贝塞尔: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
static double bezier3(double p0, double p1, double p2, double p3, double t) {
    double u = 1.0 - t;
    return u * u * u * p0 + 3.0 * u * u * t * p1 +
           3.0 * u * t * t * p2 + t * t * t * p3;
}

// 速度曲线建模 — 加速 → 匀速 → 减速
// 返回规范化后的时间映射 t_norm → t_warped
static double speed_profile(double t, double accel_ratio, double decel_ratio) {
    // accel_ratio: 加速段占比 (0.0~0.3)
    // decel_ratio: 减速段占比 (0.0~0.3)
    // 中间段: 匀速

    if (t < accel_ratio) {
        // 加速段: 0 → 1 平方加速 (ease-in)
        double p = t / accel_ratio;
        return accel_ratio * (p * p) / (accel_ratio + (1.0 - accel_ratio - decel_ratio) + decel_ratio * 0.5);
    } else if (t > 1.0 - decel_ratio) {
        // 减速段: ease-out
        double p = (t - (1.0 - decel_ratio)) / decel_ratio;
        double decel_dist = decel_ratio * (1.0 - 0.5 * 0.5) / (accel_ratio + (1.0 - accel_ratio - decel_ratio) + decel_ratio * 0.5);
        // Fitts' Law: 越接近终点越慢
        double fitts_factor = 1.0 - (1.0 - p) * (1.0 - p) * 0.3;
        double raw = (1.0 - (1.0 - p) * (1.0 - p)) * fitts_factor;
        return 1.0 - decel_ratio * (1.0 - raw);
    } else {
        // 匀速段
        double mid_start = accel_ratio;
        double mid_duration = 1.0 - accel_ratio - decel_ratio;
        double p = (t - mid_start) / mid_duration;
        return accel_ratio + p * mid_duration;
    }
}

// 均匀随机噪声 [-amplitude, +amplitude]
static int16_t random_jitter(int16_t amplitude) {
    if (amplitude <= 0) return 0;
    int range = (int)amplitude * 2 + 1;
    return (int16_t)((rand() % range) - amplitude);
}

// 生成路径点
int trajectory_generate_path(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              uint32_t duration_ms,
                              trajectory_type_t type,
                              int16_t** out_path,
                              uint32_t* out_count) {
    if (out_path == NULL || out_count == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    if (duration_ms == 0) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    // 帧率: ~125Hz (匹配 USB HID 默认 polling rate)
    #define HID_FRAME_INTERVAL_MS 8
    uint32_t frame_count = (duration_ms + HID_FRAME_INTERVAL_MS - 1) / HID_FRAME_INTERVAL_MS;
    if (frame_count < 2) frame_count = 2;
    if (frame_count > 10000) frame_count = 10000; // 安全上限

    int16_t* path = (int16_t*)malloc(frame_count * 2 * sizeof(int16_t));
    if (path == NULL) {
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    // 随机控制点偏移量 (路径长度的 10%-30%)
    double total_dist = sqrt((double)(x1 - x0) * (x1 - x0) + (double)(y1 - y0) * (y1 - y0));
    double cp_offset = total_dist * (0.1 + (double)rand() / RAND_MAX * 0.2);

    // 随机角度
    double cp_angle = ((double)rand() / RAND_MAX) * 2.0 * 3.1415926535;
    int16_t cp1_dx = (int16_t)(cos(cp_angle) * cp_offset);
    int16_t cp1_dy = (int16_t)(sin(cp_angle) * cp_offset);

    cp_angle = ((double)rand() / RAND_MAX) * 2.0 * 3.1415926535;
    int16_t cp2_dx = (int16_t)(cos(cp_angle) * cp_offset);
    int16_t cp2_dy = (int16_t)(sin(cp_angle) * cp_offset);

    // 加速/减速比 (随机化)
    double accel_ratio = 0.15 + ((double)rand() / RAND_MAX) * 0.10; // 15%-25%
    double decel_ratio = 0.20 + ((double)rand() / RAND_MAX) * 0.10; // 20%-30% (减速比加速长)

    // 生成每帧坐标
    for (uint32_t i = 0; i < frame_count; i++) {
        double t_raw = (double)i / (double)(frame_count - 1);
        double t_speed = speed_profile(t_raw, accel_ratio, decel_ratio);

        double px, py;
        switch (type) {
            case TRAJECTORY_LINEAR:
                px = lerp(x0, x1, t_speed);
                py = lerp(y0, y1, t_speed);
                break;

            case TRAJECTORY_BEZIER2:
                px = bezier2(x0, x0 + cp1_dx, x1, t_speed);
                py = bezier2(y0, y0 + cp1_dy, y1, t_speed);
                break;

            case TRAJECTORY_BEZIER3:
            default:
                px = bezier3(x0, x0 + cp1_dx, x1 + cp2_dx, x1, t_speed);
                py = bezier3(y0, y0 + cp1_dy, y1 + cp2_dy, y1, t_speed);
                break;
        }

        // Fitts' Law 修正: 接近终点时降速
        double remaining_ratio = 1.0 - t_raw;
        if (remaining_ratio < 0.15) {
            double fitts_scale = 0.5 + 0.5 * (remaining_ratio / 0.15);
            double px_end = (type == TRAJECTORY_LINEAR) ?
                lerp(x0, x1, t_speed) : bezier3(x0, x0 + cp1_dx, x1 + cp2_dx, x1, t_speed);
            double py_end = (type == TRAJECTORY_LINEAR) ?
                lerp(y0, y1, t_speed) : bezier3(y0, y0 + cp1_dy, y1 + cp2_dy, y1, t_speed);
            px = px_end * (1.0 - fitts_scale) + x1 * fitts_scale;
            py = py_end * (1.0 - fitts_scale) + y1 * fitts_scale;
        }

        // 微抖动
        int16_t jitter_x = random_jitter(2);
        int16_t jitter_y = random_jitter(2);

        path[i * 2]     = (int16_t)px + jitter_x;
        path[i * 2 + 1] = (int16_t)py + jitter_y;
    }

    // 确保终点精确到达
    path[(frame_count - 1) * 2]     = x1;
    path[(frame_count - 1) * 2 + 1] = y1;

    *out_path = path;
    *out_count = frame_count;
    return AUTOXYQ_OK;
}
```

---

### Task 13: 动作队列

**Files:**
- Create: `G:\ai\autoxyq\lib\action-queue.h`
- Create: `G:\ai\autoxyq\lib\action-queue.c`

**Interfaces:**
- Consumes: `lib/autoxyq-error.h` (Task 3)
- Produces:
  - `action_queue_init(size_t capacity)` — 创建队列
  - `action_queue_push(ActionItem* item)` — 入队
  - `action_queue_flush(void)` — 等待队列清空
  - `action_queue_destroy(void)` — 销毁队列

- [ ] **Step 1: 创建 action-queue.h**

```c
#ifndef ACTION_QUEUE_H
#define ACTION_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "autoxyq-error.h"

// 动作类型
typedef enum {
    ACTION_KEY_DOWN,
    ACTION_KEY_UP,
    ACTION_MOUSE_MOVE,
    ACTION_MOUSE_BUTTON_DOWN,
    ACTION_MOUSE_BUTTON_UP,
    ACTION_MOUSE_SCROLL,
    ACTION_MOUSE_MOVE_TO,
    ACTION_FLUSH_MARKER,
} action_type_t;

// 动作队列项
typedef struct {
    action_type_t type;
    union {
        struct { uint8_t key; uint32_t duration_ms; } key_data;
        struct { int16_t dx; int16_t dy; uint32_t duration_ms; int traj_type; } move_data;
        struct { int x; int y; uint32_t duration_ms; int traj_type; } move_to_data;
        struct { uint8_t button; } button_data;
        struct { int8_t delta; } scroll_data;
    };
} action_item_t;

// 初始化动作队列
int action_queue_init(size_t capacity);

// 入队 (生产者)
int action_queue_push(const action_item_t* item);

// 阻塞等待队列清空
void action_queue_flush(void);

// 销毁队列
void action_queue_destroy(void);

#endif // ACTION_QUEUE_H
```

- [ ] **Step 2: 创建 action-queue.c**

```c
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "action-queue.h"

// 环形队列
static action_item_t* g_QueueBuffer = NULL;
static size_t g_QueueCapacity = 0;
static volatile size_t g_QueueHead = 0;  // 生产者写入位置
static volatile size_t g_QueueTail = 0;  // 消费者读取位置
static volatile size_t g_QueueCount = 0;

// 同步原语
static CRITICAL_SECTION g_QueueLock;
static HANDLE g_QueueNotEmptyEvent = NULL;
static HANDLE g_WorkerThread = NULL;
static volatile bool g_WorkerRunning = false;
static volatile bool g_FlushRequested = false;

// 消费者工作线程
static DWORD WINAPI queue_worker_thread(LPVOID param) {
    (void)param;

    while (g_WorkerRunning) {
        // 等待非空
        WaitForSingleObject(g_QueueNotEmptyEvent, 100);

        EnterCriticalSection(&g_QueueLock);

        while (g_QueueCount > 0 && g_WorkerRunning) {
            action_item_t item = g_QueueBuffer[g_QueueTail];
            g_QueueTail = (g_QueueTail + 1) % g_QueueCapacity;
            g_QueueCount--;

            LeaveCriticalSection(&g_QueueLock);

            // 处理动作 — 调用 lib/autoxyq.c 中的对应函数
            // 由外部链接: autoxyq_key_down, autoxyq_mouse_move 等
            switch (item.type) {
                case ACTION_KEY_DOWN:
                    autoxyq_key_down(item.key_data.key);
                    break;
                case ACTION_KEY_UP:
                    autoxyq_key_up(item.key_data.key);
                    break;
                case ACTION_MOUSE_MOVE:
                    autoxyq_mouse_move_ex(item.move_data.dx, item.move_data.dy,
                        item.move_data.duration_ms,
                        (trajectory_type_t)item.move_data.traj_type);
                    break;
                case ACTION_MOUSE_BUTTON_DOWN:
                    autoxyq_mouse_button_down(item.button_data.button);
                    break;
                case ACTION_MOUSE_BUTTON_UP:
                    autoxyq_mouse_button_up(item.button_data.button);
                    break;
                case ACTION_MOUSE_SCROLL:
                    autoxyq_mouse_scroll(item.scroll_data.delta);
                    break;
                case ACTION_MOUSE_MOVE_TO:
                    autoxyq_mouse_move_to(item.move_to_data.x, item.move_to_data.y,
                        item.move_to_data.duration_ms,
                        (trajectory_type_t)item.move_to_data.traj_type);
                    break;
                case ACTION_FLUSH_MARKER:
                    g_FlushRequested = false;
                    break;
            }

            EnterCriticalSection(&g_QueueLock);
        }

        LeaveCriticalSection(&g_QueueLock);
    }

    return 0;
}

int action_queue_init(size_t capacity) {
    if (g_QueueBuffer != NULL) {
        return AUTOXYQ_OK; // 已初始化
    }

    g_QueueCapacity = (capacity > 0) ? capacity : 256;
    g_QueueBuffer = (action_item_t*)calloc(g_QueueCapacity, sizeof(action_item_t));
    if (g_QueueBuffer == NULL) {
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    InitializeCriticalSection(&g_QueueLock);

    g_QueueNotEmptyEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (g_QueueNotEmptyEvent == NULL) {
        free(g_QueueBuffer);
        g_QueueBuffer = NULL;
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    g_QueueHead = 0;
    g_QueueTail = 0;
    g_QueueCount = 0;
    g_WorkerRunning = true;

    g_WorkerThread = CreateThread(NULL, 0, queue_worker_thread, NULL, 0, NULL);
    if (g_WorkerThread == NULL) {
        CloseHandle(g_QueueNotEmptyEvent);
        free(g_QueueBuffer);
        g_QueueBuffer = NULL;
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    return AUTOXYQ_OK;
}

int action_queue_push(const action_item_t* item) {
    if (item == NULL || g_QueueBuffer == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    EnterCriticalSection(&g_QueueLock);

    if (g_QueueCount >= g_QueueCapacity) {
        LeaveCriticalSection(&g_QueueLock);
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    g_QueueBuffer[g_QueueHead] = *item;
    g_QueueHead = (g_QueueHead + 1) % g_QueueCapacity;
    g_QueueCount++;

    LeaveCriticalSection(&g_QueueLock);

    SetEvent(g_QueueNotEmptyEvent);
    return AUTOXYQ_OK;
}

void action_queue_flush(void) {
    if (g_QueueBuffer == NULL) return;

    action_item_t marker;
    marker.type = ACTION_FLUSH_MARKER;
    g_FlushRequested = true;
    action_queue_push(&marker);

    // 忙等待直到 flush marker 被处理
    while (g_FlushRequested) {
        Sleep(1);
    }
}

void action_queue_destroy(void) {
    g_WorkerRunning = false;

    if (g_WorkerThread != NULL) {
        SetEvent(g_QueueNotEmptyEvent);
        WaitForSingleObject(g_WorkerThread, 5000);
        CloseHandle(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    if (g_QueueNotEmptyEvent != NULL) {
        CloseHandle(g_QueueNotEmptyEvent);
        g_QueueNotEmptyEvent = NULL;
    }

    DeleteCriticalSection(&g_QueueLock);

    free(g_QueueBuffer);
    g_QueueBuffer = NULL;
    g_QueueCapacity = 0;
}
```

---

### Task 14: 用户态库 CMakeLists

**Files:**
- Create: `G:\ai\autoxyq\lib\CMakeLists.txt`
- Modify: `G:\ai\autoxyq\CMakeLists.txt` (启用 lib 子目录)

**Interfaces:**
- Consumes: 所有 lib/ 下源文件 (Task 9-13)
- Produces: `autoxyq.dll` + `autoxyq.lib` 编译目标

- [ ] **Step 1: 创建 lib/CMakeLists.txt**

```cmake
set(LIB_SOURCES
    autoxyq.c
    ioctl-client.c
    delay.c
    trajectory.c
    action-queue.c
)

# 动态库
add_library(autoxyq SHARED ${LIB_SOURCES})

target_compile_definitions(autoxyq PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

target_include_directories(autoxyq PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(autoxyq
    setupapi.lib
)

# 同时生成静态库供 CLI 测试使用
add_library(autoxyq-static STATIC ${LIB_SOURCES})

target_compile_definitions(autoxyq-static PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

target_include_directories(autoxyq-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

- [ ] **Step 2: 更新根 CMakeLists.txt** — 取消 `add_subdirectory(lib)` 注释

---

### Task 15: CLI 命令行工具

**Files:**
- Create: `G:\ai\autoxyq\cli\autoxyq-cli.c`
- Create: `G:\ai\autoxyq\cli\CMakeLists.txt`
- Modify: `G:\ai\autoxyq\CMakeLists.txt` (启用 cli 子目录)

**Interfaces:**
- Consumes: `lib/autoxyq.h` (Task 9)
- Produces: `autoxyq-cli.exe`

- [ ] **Step 1: 创建 cli/CMakeLists.txt**

```cmake
add_executable(autoxyq-cli
    autoxyq-cli.c
)

target_link_libraries(autoxyq-cli
    autoxyq-static
)

target_include_directories(autoxyq-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)
```

- [ ] **Step 2: 创建 autoxyq-cli.c**

```c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "autoxyq.h"

// 简易 CLI — 接收命令参数
// 用法:
//   autoxyq-cli key <usb_usage_id> [duration_ms]
//   autoxyq-cli move <dx> <dy> [duration_ms] [trajectory]
//   autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]
//   autoxyq-cli button <1-5> <down|up>
//   autoxyq-cli scroll <delta>
//   autoxyq-cli delay <min_ms> <max_ms>
//   autoxyq-cli reset

static void print_usage(void) {
    printf("autoxyq-cli - Virtual HID Keyboard & Mouse CLI\n");
    printf("Usage:\n");
    printf("  autoxyq-cli key <usb_usage_id> [duration_ms]\n");
    printf("  autoxyq-cli move <dx> <dy> [duration_ms] [trajectory:0=linear,1=bezier2,2=bezier3]\n");
    printf("  autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]\n");
    printf("  autoxyq-cli button <1-5> <down|up>\n");
    printf("  autoxyq-cli scroll <delta>\n");
    printf("  autoxyq-cli delay <min_ms> <max_ms>\n");
    printf("  autoxyq-cli reset\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    int ret = autoxyq_init();
    if (ret != AUTOXYQ_OK) {
        fprintf(stderr, "Error: %s\n", autoxyq_strerror(ret));
        return ret;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "key") == 0 && argc >= 3) {
        int key = atoi(argv[2]);
        int duration = (argc >= 4) ? atoi(argv[3]) : 0;
        if (duration > 0) {
            ret = autoxyq_key_press((uint8_t)key, (uint32_t)duration);
        } else {
            ret = autoxyq_key_down((uint8_t)key);
            autoxyq_flush();
            ret = autoxyq_key_up((uint8_t)key);
        }
        printf("Key: 0x%02X duration=%dms -> %s\n", key, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "move") == 0 && argc >= 4) {
        int dx = atoi(argv[2]);
        int dy = atoi(argv[3]);
        int duration = (argc >= 5) ? atoi(argv[4]) : 0;
        int traj = (argc >= 6) ? atoi(argv[5]) : TRAJECTORY_BEZIER3;
        ret = autoxyq_mouse_move_ex((int16_t)dx, (int16_t)dy,
            (uint32_t)duration, (trajectory_type_t)traj);
        printf("Move: dx=%d dy=%d duration=%dms -> %s\n", dx, dy, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "moveto") == 0 && argc >= 5) {
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int duration = atoi(argv[4]);
        int traj = (argc >= 6) ? atoi(argv[5]) : TRAJECTORY_BEZIER3;
        ret = autoxyq_mouse_move_to(x, y, (uint32_t)duration, (trajectory_type_t)traj);
        printf("MoveTo: x=%d y=%d duration=%dms -> %s\n", x, y, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "button") == 0 && argc >= 4) {
        int button = atoi(argv[2]);
        if (strcmp(argv[3], "down") == 0) {
            ret = autoxyq_mouse_button_down((uint8_t)button);
        } else {
            ret = autoxyq_mouse_button_up((uint8_t)button);
        }
        printf("Button %d %s -> %s\n", button, argv[3], autoxyq_strerror(ret));

    } else if (strcmp(cmd, "scroll") == 0 && argc >= 3) {
        int delta = atoi(argv[2]);
        ret = autoxyq_mouse_scroll((int8_t)delta);
        printf("Scroll: %d -> %s\n", delta, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "delay") == 0 && argc >= 4) {
        int min_ms = atoi(argv[2]);
        int max_ms = atoi(argv[3]);
        autoxyq_set_delay_range((uint32_t)min_ms, (uint32_t)max_ms);
        printf("Delay range: [%d, %d] ms\n", min_ms, max_ms);

    } else if (strcmp(cmd, "reset") == 0) {
        ret = autoxyq_mouse_button_up(1);
        ret = autoxyq_mouse_button_up(2);
        ret = autoxyq_mouse_button_up(3);
        ret = autoxyq_mouse_button_up(4);
        ret = autoxyq_mouse_button_up(5);
        printf("Reset -> %s\n", autoxyq_strerror(ret));

    } else {
        print_usage();
    }

    autoxyq_flush();
    autoxyq_shutdown();
    return ret;
}
```

- [ ] **Step 3: 更新根 CMakeLists.txt** — 取消 `add_subdirectory(cli)` 注释

---

### Task 16: 驱动安装/卸载脚本

**Files:**
- Create: `G:\ai\autoxyq\scripts\install-driver.ps1`
- Create: `G:\ai\autoxyq\scripts\uninstall-driver.ps1`

**Interfaces:**
- Consumes: (独立脚本)
- Produces: 驱动安装/卸载 PowerShell 脚本

- [ ] **Step 1: 创建 install-driver.ps1**

```powershell
# install-driver.ps1 — 安装 autoxyq 虚拟 HID 驱动
# 需要管理员权限

param(
    [string]$InfPath = ".\autoxyq-drv.inx"
)

$ErrorActionPreference = "Stop"

# 检查管理员权限
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "This script requires Administrator privileges. Restarting with elevation..."
    Start-Process powershell -Verb RunAs -ArgumentList "-File `"$PSCommandPath`" -InfPath `"$InfPath`""
    exit
}

Write-Host "=== autoxyq Driver Installation ==="

$infFullPath = Resolve-Path $InfPath -ErrorAction Stop
Write-Host "INF: $infFullPath"

# 安装驱动
Write-Host "Installing driver..."
pnputil /add-driver "$infFullPath" /install

Write-Host "Driver installation complete."
Write-Host "Verify: Open Device Manager and look for 'autoxyq Virtual HID Device' under 'Human Interface Devices'"
```

- [ ] **Step 2: 创建 uninstall-driver.ps1**

```powershell
# uninstall-driver.ps1 — 卸载 autoxyq 虚拟 HID 驱动
# 需要管理员权限

param(
    [string]$DriverName = "autoxyq-drv.inf"
)

$ErrorActionPreference = "Stop"

if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Start-Process powershell -Verb RunAs -ArgumentList "-File `"$PSCommandPath`" -DriverName `"$DriverName`""
    exit
}

Write-Host "=== autoxyq Driver Uninstallation ==="

# 查找已安装的驱动
$installedDriver = pnputil /enum-drivers | Select-String $DriverName

if ($installedDriver) {
    Write-Host "Removing driver..."
    pnputil /delete-driver $DriverName /uninstall
    Write-Host "Driver removed."
} else {
    Write-Host "Driver not found (already uninstalled?)."
}
```

---

### Task 17: 单元测试

**Files:**
- Create: `G:\ai\autoxyq\tests\CMakeLists.txt`
- Create: `G:\ai\autoxyq\tests\test-trajectory.c`
- Create: `G:\ai\autoxyq\tests\test-delay.c`
- Create: `G:\ai\autoxyq\tests\test-queue.c`
- Modify: `G:\ai\autoxyq\CMakeLists.txt` (启用 tests 子目录 + enable_testing)

**Interfaces:**
- Consumes: `lib/` 纯算法模块 (Task 11, 12, 13)
- Produces: 3 个可执行测试目标, ctest 集成

- [ ] **Step 1: 创建 tests/CMakeLists.txt**

```cmake
enable_testing()

# test-delay
add_executable(test-delay
    test-delay.c
    ${CMAKE_SOURCE_DIR}/lib/delay.c
)
target_include_directories(test-delay PRIVATE ${CMAKE_SOURCE_DIR}/lib)
add_test(NAME test-delay COMMAND test-delay)

# test-trajectory
add_executable(test-trajectory
    test-trajectory.c
    ${CMAKE_SOURCE_DIR}/lib/trajectory.c
    ${CMAKE_SOURCE_DIR}/lib/delay.c
)
target_include_directories(test-trajectory PRIVATE ${CMAKE_SOURCE_DIR}/lib)
add_test(NAME test-trajectory COMMAND test-trajectory)

# test-queue
add_executable(test-queue
    test-queue.c
    ${CMAKE_SOURCE_DIR}/lib/action-queue.c
)
target_include_directories(test-queue PRIVATE ${CMAKE_SOURCE_DIR}/lib)
add_test(NAME test-queue COMMAND test-queue)
```

- [ ] **Step 2: 创建 test-delay.c**

```c
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include "delay.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  TEST: %s ... ", name); \
    tests_run++; \
} while(0)

#define PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
} while(0)

void test_delay_range_default(void) {
    TEST("default delay range returns valid values");
    for (int i = 0; i < 100; i++) {
        uint32_t d = delay_random_ms();
        if (d < 10 || d > 50) {
            FAIL("delay out of default range [10, 50]");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_range_custom(void) {
    TEST("custom delay range [100, 200]");
    delay_set_range(100, 200);
    for (int i = 0; i < 100; i++) {
        uint32_t d = delay_random_ms();
        if (d < 100 || d > 200) {
            FAIL("delay out of custom range");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_min_equals_max(void) {
    TEST("min equals max returns exact value");
    delay_set_range(42, 42);
    for (int i = 0; i < 20; i++) {
        uint32_t d = delay_random_ms();
        if (d != 42) {
            FAIL("delay should be exactly 42");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_sleep_accuracy(void) {
    TEST("sleep_us accuracy within tolerance");
    DWORD before = GetTickCount();
    delay_sleep_us(100000); // 100ms
    DWORD after = GetTickCount();
    DWORD elapsed = after - before;
    if (elapsed < 90 || elapsed > 150) {
        FAIL("sleep accuracy out of tolerance");
        printf("     expected ~100ms, got %lums\n", elapsed);
        return;
    }
    PASS();
}

int main(void) {
    printf("=== delay engine tests ===\n");
    test_delay_range_default();
    test_delay_range_custom();
    test_delay_min_equals_max();
    test_delay_sleep_accuracy();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
```

- [ ] **Step 3: 创建 test-trajectory.c**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "trajectory.h"
#include "autoxyq-error.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

// 外部变量 (trajectory.c 需要)
uint8_t g_JitterAmplitude = 0;  // 测试时关闭抖动
uint32_t g_RandomSeed = 12345;

void test_trajectory_linear(void) {
    TEST("linear trajectory generates path");
    int16_t* path = NULL;
    uint32_t count = 0;
    int ret = trajectory_generate_path(0, 0, 100, 0, 200,
        TRAJECTORY_LINEAR, &path, &count);
    if (ret != AUTOXYQ_OK) {
        FAIL("trajectory_generate_path failed");
        return;
    }
    if (count < 2) {
        FAIL("path too short");
        free(path);
        return;
    }
    // 终点精确到达
    if (path[(count - 1) * 2] != 100 || path[(count - 1) * 2 + 1] != 0) {
        FAIL("endpoint mismatch");
        free(path);
        return;
    }
    free(path);
    PASS();
}

void test_trajectory_bezier3(void) {
    TEST("bezier3 trajectory generates path");
    int16_t* path = NULL;
    uint32_t count = 0;
    int ret = trajectory_generate_path(0, 0, 200, -100, 500,
        TRAJECTORY_BEZIER3, &path, &count);
    if (ret != AUTOXYQ_OK) {
        FAIL("trajectory_generate_path failed");
        return;
    }
    if (count < 2) {
        FAIL("path too short");
        free(path);
        return;
    }
    // 终点精确到达
    if (path[(count - 1) * 2] != 200 || path[(count - 1) * 2 + 1] != -100) {
        FAIL("endpoint mismatch");
        free(path);
        return;
    }
    free(path);
    PASS();
}

void test_trajectory_invalid_params(void) {
    TEST("invalid parameters return error");
    int16_t* path = NULL;
    uint32_t count = 0;

    int ret = trajectory_generate_path(0, 0, 0, 0, 0,
        TRAJECTORY_LINEAR, &path, &count);
    if (ret != AUTOXYQ_ERR_INVALID_PARAM) {
        FAIL("should return INVALID_PARAM for zero duration");
        return;
    }

    ret = trajectory_generate_path(0, 0, 0, 0, 100,
        TRAJECTORY_LINEAR, NULL, &count);
    if (ret != AUTOXYQ_ERR_INVALID_PARAM) {
        FAIL("should return INVALID_PARAM for NULL output");
        return;
    }

    PASS();
}

int main(void) {
    srand(12345);
    printf("=== trajectory engine tests ===\n");
    test_trajectory_linear();
    test_trajectory_bezier3();
    test_trajectory_invalid_params();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
```

- [ ] **Step 4: 创建 test-queue.c**

```c
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "action-queue.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

void test_queue_init(void) {
    TEST("queue initialization");
    int ret = action_queue_init(64);
    if (ret != AUTOXYQ_OK) {
        FAIL("init failed");
        return;
    }
    action_queue_destroy();
    PASS();
}

void test_queue_push(void) {
    TEST("queue push does not fail");
    action_queue_init(8);
    action_item_t item;
    item.type = ACTION_KEY_DOWN;
    item.key_data.key = 0x04; // 'A'
    item.key_data.duration_ms = 0;

    int ret = action_queue_push(&item);
    if (ret != AUTOXYQ_OK) {
        FAIL("push failed");
        action_queue_destroy();
        return;
    }

    // 给工作线程一点时间处理
    Sleep(100);
    action_queue_destroy();
    PASS();
}

void test_queue_full(void) {
    TEST("queue full returns error");
    action_queue_init(2);

    action_item_t item;
    item.type = ACTION_KEY_DOWN;
    item.key_data.key = 0x04;
    item.key_data.duration_ms = 0;

    action_queue_push(&item);
    action_queue_push(&item);
    int ret = action_queue_push(&item);

    if (ret != AUTOXYQ_ERR_QUEUE_FULL) {
        FAIL("should return QUEUE_FULL");
        action_queue_destroy();
        return;
    }

    Sleep(200);
    action_queue_destroy();
    PASS();
}

int main(void) {
    printf("=== action queue tests ===\n");
    test_queue_init();
    test_queue_push();
    test_queue_full();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
```

- [ ] **Step 5: 更新根 CMakeLists.txt** — 取消 `add_subdirectory(tests)` 注释，添加 `enable_testing()`

---

### Task 18: README 完善

**Files:**
- Modify: `G:\ai\autoxyq\README.md` (完善自 Task 1 骨架)

- [ ] **Step 1: 完善 README.md**

```markdown
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
```

- [ ] **Step 2: 更新根 CMakeLists.txt** — 确保所有 `add_subdirectory` 已启用

最终的根 `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.21)
project(autoxyq VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /wd4100 /wd4201 /wd4214)
endif()

enable_testing()

add_subdirectory(driver)
add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(tests)
```

---

## 实施依赖顺序

```
Task 1 (骨架) ──▶ Task 2 (CI)    可并行
             ──▶ Task 3 (共享定义) ──▶ Task 4 (驱动骨架) ──▶ Task 5 (键盘)
                                                           ──▶ Task 6 (鼠标)
                                                           ──▶ Task 7 (IOCTL)
                                                           ──▶ Task 8 (INF+CMake)
             ──▶ Task 9 (API头)  ──▶ Task 10 (库入口)   ──▶ Task 11 (延迟)
                                                           Task 12 (轨迹)   ──▶ Task 13 (队列)
                                                                                ──▶ Task 14 (lib CMake)
                                                                                     Task 15 (CLI)
                                                                                     Task 16 (脚本) (独立)
                                                                                     Task 17 (测试)
                                                                                     Task 18 (README)
```

## 总共 18 个任务

建议执行顺序: 1, 2, 3 → 4, 5, 6, 7, 8 (驱动部分) || 9, 10, 11, 12, 13, 14 (库部分) → 15, 16, 17, 18
