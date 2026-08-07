#include "autoxyq-drv.h"
#include "ioctl-defs.h"
#include "hid-report-desc.h"
#include "trace.h"

extern VOID EvtIoDeviceControl(_In_ WDFQUEUE, _In_ WDFREQUEST, _In_ size_t, _In_ size_t, _In_ ULONG);
extern void HidKeyboard_Init(void);
extern void HidMouse_Init(void);

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
    ctx->VhidHandle = INVALID_HANDLE_VALUE;

    // 初始化 HID 模块
    HidKeyboard_Init();
    HidMouse_Init();

    // 创建默认 IOCTL 队列
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

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

// 硬件准备 — 打开 vhidmini 设备
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

// 硬件释放 — 关闭 vhidmini 句柄
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
