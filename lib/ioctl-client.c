#include <windows.h>
#include <winioctl.h>   // CTL_CODE (ioctl-defs.h 中的 IOCTL 常量)
#include <stdint.h>
#include <string.h>
#include "ioctl-defs.h"
#include "autoxyq-error.h"

// ============================================================
// SendInput 后端
//
// 将 HID 逻辑 Report 翻译为 Windows 用户态合成输入 (SendInput)。
// 与原 IOCTL 后端不同, 这里没有真实驱动设备, 输入直接注入系统队列。
//
// 注意: SendInput 注入的输入带 LLMHF_INJECTED 标志, 可被反作弊识别,
// 仅用于自动化测试 / 学习用途。
// ============================================================

// ---- 键盘映射: USB HID Usage ID (Keyboard/Keypad page) → PS/2 Set1 扫描码 ----
// 用扫描码 + KEYEVENTF_SCANCODE 注入, 不依赖当前键盘布局 (与 HID 的位置语义一致)。
// 数组下标 = HID Usage ID, 范围 0x00-0x65。
typedef struct {
    uint8_t scancode;   // Set1 make code (低字节)
    uint8_t e0;         // 1 = E0 扩展键 (附加 KEYEVENTF_EXTENDEDKEY)
    uint8_t vk;         // 0 = 用扫描码; 非 0 = 用虚拟键码 (多字节序列的特殊键)
} keymap_entry_t;

static const keymap_entry_t g_Keymap[0x66] = {
    {0x00, 0, 0},  // 0x00 Reserved
    {0x00, 0, 0},  // 0x01 ErrorRollOver
    {0x00, 0, 0},  // 0x02 POSTFail
    {0x00, 0, 0},  // 0x03 ErrorUndefined
    {0x1E, 0, 0},  // 0x04 A
    {0x30, 0, 0},  // 0x05 B
    {0x2E, 0, 0},  // 0x06 C
    {0x20, 0, 0},  // 0x07 D
    {0x12, 0, 0},  // 0x08 E
    {0x21, 0, 0},  // 0x09 F
    {0x22, 0, 0},  // 0x0A G
    {0x23, 0, 0},  // 0x0B H
    {0x17, 0, 0},  // 0x0C I
    {0x24, 0, 0},  // 0x0D J
    {0x25, 0, 0},  // 0x0E K
    {0x26, 0, 0},  // 0x0F L
    {0x32, 0, 0},  // 0x10 M
    {0x31, 0, 0},  // 0x11 N
    {0x18, 0, 0},  // 0x12 O
    {0x19, 0, 0},  // 0x13 P
    {0x10, 0, 0},  // 0x14 Q
    {0x13, 0, 0},  // 0x15 R
    {0x1F, 0, 0},  // 0x16 S
    {0x14, 0, 0},  // 0x17 T
    {0x16, 0, 0},  // 0x18 U
    {0x2F, 0, 0},  // 0x19 V
    {0x11, 0, 0},  // 0x1A W
    {0x2D, 0, 0},  // 0x1B X
    {0x15, 0, 0},  // 0x1C Y
    {0x2C, 0, 0},  // 0x1D Z
    {0x02, 0, 0},  // 0x1E 1
    {0x03, 0, 0},  // 0x1F 2
    {0x04, 0, 0},  // 0x20 3
    {0x05, 0, 0},  // 0x21 4
    {0x06, 0, 0},  // 0x22 5
    {0x07, 0, 0},  // 0x23 6
    {0x08, 0, 0},  // 0x24 7
    {0x09, 0, 0},  // 0x25 8
    {0x0A, 0, 0},  // 0x26 9
    {0x0B, 0, 0},  // 0x27 0
    {0x1C, 0, 0},  // 0x28 Enter
    {0x01, 0, 0},  // 0x29 Escape
    {0x0E, 0, 0},  // 0x2A Backspace
    {0x0F, 0, 0},  // 0x2B Tab
    {0x39, 0, 0},  // 0x2C Space
    {0x0C, 0, 0},  // 0x2D - _
    {0x0D, 0, 0},  // 0x2E = +
    {0x1A, 0, 0},  // 0x2F [ {
    {0x1B, 0, 0},  // 0x30 ] }
    {0x2B, 0, 0},  // 0x31 \ |
    {0x56, 0, 0},  // 0x32 Non-US # ~
    {0x27, 0, 0},  // 0x33 ; :
    {0x28, 0, 0},  // 0x34 ' "
    {0x29, 0, 0},  // 0x35 ` ~
    {0x33, 0, 0},  // 0x36 , <
    {0x34, 0, 0},  // 0x37 . >
    {0x35, 0, 0},  // 0x38 / ?
    {0x3A, 0, 0},  // 0x39 Caps Lock
    {0x3B, 0, 0},  // 0x3A F1
    {0x3C, 0, 0},  // 0x3B F2
    {0x3D, 0, 0},  // 0x3C F3
    {0x3E, 0, 0},  // 0x3D F4
    {0x3F, 0, 0},  // 0x3E F5
    {0x40, 0, 0},  // 0x3F F6
    {0x41, 0, 0},  // 0x40 F7
    {0x42, 0, 0},  // 0x41 F8
    {0x43, 0, 0},  // 0x42 F9
    {0x44, 0, 0},  // 0x43 F10
    {0x57, 0, 0},  // 0x44 F11
    {0x58, 0, 0},  // 0x45 F12
    {0x00, 0, 0x2C},  // 0x46 PrintScreen (VK_SNAPSHOT, Set1 多字节序列)
    {0x46, 0, 0},  // 0x47 Scroll Lock
    {0x00, 0, 0x13},  // 0x48 Pause (VK_PAUSE, Set1 多字节序列)
    {0x52, 1, 0},  // 0x49 Insert (E0 52)
    {0x47, 1, 0},  // 0x4A Home (E0 47)
    {0x49, 1, 0},  // 0x4B Page Up (E0 49)
    {0x53, 1, 0},  // 0x4C Delete (E0 53)
    {0x4F, 1, 0},  // 0x4D End (E0 4F)
    {0x51, 1, 0},  // 0x4E Page Down (E0 51)
    {0x4D, 1, 0},  // 0x4F Right Arrow (E0 4D)
    {0x4B, 1, 0},  // 0x50 Left Arrow (E0 4B)
    {0x50, 1, 0},  // 0x51 Down Arrow (E0 50)
    {0x48, 1, 0},  // 0x52 Up Arrow (E0 48)
    {0x45, 0, 0},  // 0x53 Num Lock
    {0x35, 1, 0},  // 0x54 Keypad / (E0 35)
    {0x37, 0, 0},  // 0x55 Keypad *
    {0x4A, 0, 0},  // 0x56 Keypad -
    {0x4E, 0, 0},  // 0x57 Keypad +
    {0x1C, 1, 0},  // 0x58 Keypad Enter (E0 1C)
    {0x4F, 0, 0},  // 0x59 Keypad 1 End
    {0x50, 0, 0},  // 0x5A Keypad 2 Down
    {0x51, 0, 0},  // 0x5B Keypad 3 PgDn
    {0x4B, 0, 0},  // 0x5C Keypad 4 Left
    {0x4C, 0, 0},  // 0x5D Keypad 5
    {0x4D, 0, 0},  // 0x5E Keypad 6 Right
    {0x47, 0, 0},  // 0x5F Keypad 7 Home
    {0x48, 0, 0},  // 0x60 Keypad 8 Up
    {0x49, 0, 0},  // 0x61 Keypad 9 PgUp
    {0x52, 0, 0},  // 0x62 Keypad 0 Ins
    {0x53, 0, 0},  // 0x63 Keypad . Del
    {0x56, 0, 0},  // 0x64 Non-US \ |
    {0x5D, 1, 0},  // 0x65 Application (E0 5D)
};

// ---- 修饰键映射: modifier_bitmap 位 → Set1 扫描码 ----
typedef struct {
    uint8_t scancode;
    uint8_t e0;
} modifier_map_entry_t;

static const modifier_map_entry_t g_ModifierMap[8] = {
    {0x1D, 0},  // bit0 LCtrl
    {0x2A, 0},  // bit1 LShift
    {0x38, 0},  // bit2 LAlt
    {0x5B, 1},  // bit3 LGUI (E0 5B)
    {0x1D, 1},  // bit4 RCtrl (E0 1D)
    {0x36, 0},  // bit5 RShift
    {0x38, 1},  // bit6 RAlt (E0 38)
    {0x5C, 1},  // bit7 RGUI (E0 5C)
};

// ---- 后端状态 (用于完整 Report → 边沿事件的 diff) ----
static keyboard_report_t g_PrevKeyboard;
static uint8_t g_PrevMouseButtons = 0;

// 发送单个键盘事件 (扫描码方式)
static void send_key_scancode(uint8_t scancode, uint8_t e0, BOOL down) {
    INPUT in;
    memset(&in, 0, sizeof(in));
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = scancode;
    in.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (e0) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (!down) in.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
}

// 发送单个键盘事件 (虚拟键码方式, 用于 PrintScreen / Pause)
static void send_key_vk(uint8_t vk, BOOL down) {
    INPUT in;
    memset(&in, 0, sizeof(in));
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
}

// 按 HID Usage ID 发送按键按下/抬起
static void send_key_by_usage(uint8_t usage, BOOL down) {
    if (usage == 0 || usage >= 0x66) {
        return; // 非法或未映射
    }
    const keymap_entry_t* e = &g_Keymap[usage];
    if (e->vk != 0) {
        send_key_vk(e->vk, down);
    } else if (e->scancode != 0) {
        send_key_scancode(e->scancode, e->e0, down);
    }
}

// 发送鼠标按键事件 (button: 0=左 1=右 2=中 3=侧1 4=侧2)
static void send_mouse_button(uint8_t button, BOOL down) {
    INPUT in;
    memset(&in, 0, sizeof(in));
    in.type = INPUT_MOUSE;
    switch (button) {
    case 0: in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN   : MOUSEEVENTF_LEFTUP;   break;
    case 1: in.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN  : MOUSEEVENTF_RIGHTUP;  break;
    case 2: in.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
    case 3: in.mi.dwFlags = down ? MOUSEEVENTF_XDOWN      : MOUSEEVENTF_XUP;
            in.mi.mouseData = XBUTTON1; break;
    case 4: in.mi.dwFlags = down ? MOUSEEVENTF_XDOWN      : MOUSEEVENTF_XUP;
            in.mi.mouseData = XBUTTON2; break;
    default: return;
    }
    SendInput(1, &in, sizeof(INPUT));
}

// ============================================================
// 对外接口 (与 autoxyq.c 的 extern 声明一致)
// ============================================================

// 发现并打开驱动设备
// SendInput 后端无需真实设备, 返回非 INVALID_HANDLE_VALUE 的占位句柄
static int g_DummyDevice = 0;

HANDLE ioctl_open_device(void) {
    // 声明 Per-Monitor V2 DPI 感知: 使 GetSystemMetrics / GetCursorPos 与
    // MOUSEEVENTF_ABSOLUTE 的归一化统一到物理像素域,
    // 消除高 DPI 及混合缩放多显示器下 moveto 绝对定位的偏移
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    return (HANDLE)&g_DummyDevice;
}

// 关闭设备
void ioctl_close_device(HANDLE hDevice) {
    (void)hDevice;
}

// 发送键盘 Report
int ioctl_send_keyboard(HANDLE hDevice, const keyboard_report_t* report) {
    (void)hDevice;

    if (report == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    if (report->reserved != 0) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    // 修饰键 diff
    for (int i = 0; i < 8; i++) {
        uint8_t mask = (uint8_t)(1u << i);
        uint8_t was = g_PrevKeyboard.modifier_bitmap & mask;
        uint8_t now = report->modifier_bitmap & mask;
        if (was != now) {
            send_key_scancode(g_ModifierMap[i].scancode, g_ModifierMap[i].e0, now != 0);
        }
    }

    // 6 键数组 diff (集合语义: 数组顺序无意义)
    // 抬起: prev 有而 new 无的键
    for (int i = 0; i < 6; i++) {
        uint8_t k = g_PrevKeyboard.key_codes[i];
        if (k == 0) continue;
        int still_pressed = 0;
        for (int j = 0; j < 6; j++) {
            if (report->key_codes[j] == k) { still_pressed = 1; break; }
        }
        if (!still_pressed) send_key_by_usage(k, FALSE);
    }
    // 按下: new 有而 prev 无的键
    for (int i = 0; i < 6; i++) {
        uint8_t k = report->key_codes[i];
        if (k == 0) continue;
        int was_pressed = 0;
        for (int j = 0; j < 6; j++) {
            if (g_PrevKeyboard.key_codes[j] == k) { was_pressed = 1; break; }
        }
        if (!was_pressed) send_key_by_usage(k, TRUE);
    }

    g_PrevKeyboard = *report;
    return AUTOXYQ_OK;
}

// 发送鼠标 Report
int ioctl_send_mouse(HANDLE hDevice, const mouse_report_t* report) {
    (void)hDevice;

    if (report == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    if (report->button_mask > 0x1F) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    uint8_t now = report->button_mask;

    // 按键 diff
    for (int b = 0; b < 5; b++) {
        uint8_t mask = (uint8_t)(1u << b);
        if ((g_PrevMouseButtons ^ now) & mask) {
            send_mouse_button((uint8_t)b, (now & mask) != 0);
        }
    }

    // 相对移动
    if (report->dx != 0 || report->dy != 0) {
        INPUT in;
        memset(&in, 0, sizeof(in));
        in.type = INPUT_MOUSE;
        in.mi.dx = report->dx;
        in.mi.dy = report->dy;
        in.mi.dwFlags = MOUSEEVENTF_MOVE; // 相对位移
        SendInput(1, &in, sizeof(INPUT));
    }

    // 垂直滚轮 (正值向上)
    if (report->wheel != 0) {
        INPUT in;
        memset(&in, 0, sizeof(in));
        in.type = INPUT_MOUSE;
        in.mi.mouseData = (DWORD)((int)report->wheel * WHEEL_DELTA);
        in.mi.dwFlags = MOUSEEVENTF_WHEEL;
        SendInput(1, &in, sizeof(INPUT));
    }

    g_PrevMouseButtons = now;
    return AUTOXYQ_OK;
}

// 发送绝对坐标移动 (屏幕像素)
// MOUSEEVENTF_ABSOLUTE 模式不受系统鼠标加速影响, 落点精确。
int ioctl_send_mouse_absolute(HANDLE hDevice, int x, int y) {
    (void)hDevice;

    // 虚拟屏幕坐标 → 0..65535 归一化绝对坐标
    int left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    INPUT in;
    memset(&in, 0, sizeof(in));
    in.type = INPUT_MOUSE;
    in.mi.dx = (LONG)((x - left) * 65536 / width);
    in.mi.dy = (LONG)((y - top) * 65536 / height);
    in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    SendInput(1, &in, sizeof(INPUT));

    return AUTOXYQ_OK;
}

// 重置所有设备 (抬起所有按下的键/按钮, 清空状态)
int ioctl_reset_devices(HANDLE hDevice) {
    (void)hDevice;

    for (int i = 0; i < 6; i++) {
        uint8_t k = g_PrevKeyboard.key_codes[i];
        if (k != 0) send_key_by_usage(k, FALSE);
    }
    for (int i = 0; i < 8; i++) {
        if (g_PrevKeyboard.modifier_bitmap & (1u << i)) {
            send_key_scancode(g_ModifierMap[i].scancode, g_ModifierMap[i].e0, FALSE);
        }
    }
    for (int b = 0; b < 5; b++) {
        if (g_PrevMouseButtons & (1u << b)) {
            send_mouse_button((uint8_t)b, FALSE);
        }
    }

    memset(&g_PrevKeyboard, 0, sizeof(g_PrevKeyboard));
    g_PrevMouseButtons = 0;
    return AUTOXYQ_OK;
}
