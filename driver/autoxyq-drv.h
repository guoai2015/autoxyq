#ifndef AUTOXYQ_DRV_H
#define AUTOXYQ_DRV_H

#include <windows.h>
#include <wdf.h>

// 设备上下文 — 挂载在 WDFDEVICE 上
typedef struct _DEVICE_CONTEXT {
    BOOLEAN IsPoweredOn;
    HANDLE  VhidHandle;   // vhidmini 设备句柄 (用于提交 HID Report)
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
