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
