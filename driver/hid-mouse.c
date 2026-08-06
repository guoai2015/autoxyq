#include <windows.h>
#include <stdint.h>
#include "hid-report-desc.h"
#include "ioctl-defs.h"   // mouse_report_t 定义

// 鼠标状态跟踪
typedef struct {
    uint8_t  button_mask;
    int16_t  dx;
    int16_t  dy;
    int8_t   wheel;
} mouse_state_t;

static mouse_state_t g_MouseState;

// 5 字节原始 HID Report, 精确匹配 MOUSE_HID_REPORT_DESC 的编码:
//   byte0: 按键 (bit0-4) + 填充 3bit (bit5-7)
//   byte1: X 低 8 位 (12-bit X 的 bit0-7)
//   byte2: X 高 4 位 (bit0-3) | Y 低 4 位 (bit4-7)
//   byte3: Y 高 8 位 (12-bit Y 的 bit4-11)
//   byte4: 滚轮 (8-bit)
// 注意: MOUSE_REPORT_SIZE 仍为 4, 那是 ioctl handler 的缓冲区大小, 由 Task 7 处理
#pragma pack(push, 1)
typedef struct {
    uint8_t  button_field;   // bit0-4: 按键, bit5-7: 填充 (0)
    uint8_t  dx_lo;          // X 低 8 位
    uint8_t  dx_hi_dy_lo;    // X 高 4 位 | Y 低 4 位
    uint8_t  dy_hi;          // Y 高 8 位
    int8_t   wheel;          // 垂直滚轮
} mouse_raw_report_t;
#pragma pack(pop)

// 原始 HID Report 长度 (5 字节)
#define MOUSE_RAW_REPORT_SIZE (sizeof(mouse_raw_report_t))

// 逻辑 Report → 5 字节原始 HID Report
// 12-bit X/Y 以二进制补码位域打包: X 占用 bit0-11, Y 占用 bit12-23
void HidMouse_LogicalToRaw(const mouse_report_t* logical, mouse_raw_report_t* raw) {
    if (logical == NULL || raw == NULL) {
        return;
    }
    // 转无符号, 避免有符号右移的符号扩展 (仅保留低 12 位有效)
    uint16_t x = (uint16_t)logical->dx;
    uint16_t y = (uint16_t)logical->dy;

    raw->button_field = (uint8_t)(logical->button_mask & 0x1F);   // 填充位清零
    raw->dx_lo        = (uint8_t)(x & 0xFF);
    raw->dx_hi_dy_lo  = (uint8_t)(((x >> 8) & 0x0F) | ((y & 0x0F) << 4));
    raw->dy_hi        = (uint8_t)((y >> 4) & 0xFF);
    raw->wheel        = logical->wheel;
}

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

    // 逻辑 Report → 5 字节原始 HID Report (精确匹配 HID descriptor 编码)
    mouse_raw_report_t rawReport;
    HidMouse_LogicalToRaw(report, &rawReport);

    // 通过 vhidmini 接口发送 5 字节 HID Report
    // vhidmini 使用 IOCTL_VHIDMINI_SUBMIT_REPORT (后续任务中定义)
    // 当前存根，等待 vhidmini 集成
    (void)vhidHandle;
    (void)rawReport;

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
