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
