#include <windows.h>
#include <stdint.h>
#include "hid-report-desc.h"
#include "ioctl-defs.h"

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
