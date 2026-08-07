#include <windows.h>
#include <setupapi.h>
#include <stdint.h>
#include "ioctl-defs.h"
#include "autoxyq-error.h"

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
