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
