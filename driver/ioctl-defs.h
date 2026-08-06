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
