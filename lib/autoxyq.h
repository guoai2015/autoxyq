#ifndef AUTOXYQ_H
#define AUTOXYQ_H

#include <stdint.h>
#include "autoxyq-error.h"

// 轨迹类型枚举
typedef enum {
    TRAJECTORY_LINEAR  = 0,
    TRAJECTORY_BEZIER2 = 1,
    TRAJECTORY_BEZIER3 = 2,
} trajectory_type_t;

// ---- 生命周期 ----

/**
 * 初始化 autoxyq 库，发现并打开驱动设备。
 * 返回 AUTOXYQ_OK 成功，否则返回错误码。
 */
int autoxyq_init(void);

/**
 * 关闭 autoxyq 库。
 * 自动清空动作队列、发送重置命令、关闭驱动句柄。
 */
void autoxyq_shutdown(void);

// ---- 键盘操作 ----

/** 按下按键 (USB HID Usage ID) */
int autoxyq_key_down(uint8_t usb_usage_id);

/** 弹起按键 (USB HID Usage ID) */
int autoxyq_key_up(uint8_t usb_usage_id);

/** 按下 + 延时 + 弹起 */
int autoxyq_key_press(uint8_t usb_usage_id, uint32_t duration_ms);

// ---- 鼠标操作 (相对位移) ----

/** 瞬间相对移动 */
int autoxyq_mouse_move(int16_t dx, int16_t dy);

/** 带轨迹的平滑相对移动 */
int autoxyq_mouse_move_ex(int16_t dx, int16_t dy, uint32_t duration_ms,
                           trajectory_type_t type);

// ---- 鼠标操作 (绝对坐标) ----

/**
 * 移动到屏幕绝对坐标。
 * 通过持续发送相对位移模拟，使用轨迹插值。
 * x, y: 目标屏幕坐标 (像素)
 * duration_ms: 移动总时长 (毫秒)
 * type: 轨迹曲线类型
 */
int autoxyq_mouse_move_to(int x, int y, uint32_t duration_ms,
                           trajectory_type_t type);

// ---- 鼠标按键 ----

/** 按下鼠标按钮 (1=左, 2=右, 3=中, 4=侧键1, 5=侧键2) */
int autoxyq_mouse_button_down(uint8_t button);

/** 弹起鼠标按钮 */
int autoxyq_mouse_button_up(uint8_t button);

/** 滚轮滚动 (正值向上, 负值向下) */
int autoxyq_mouse_scroll(int8_t delta);

// ---- 配置 ----

/** 设置全局随机延迟范围 (毫秒)，min_ms <= max_ms */
void autoxyq_set_delay_range(uint32_t min_ms, uint32_t max_ms);

/** 设置默认轨迹类型 */
void autoxyq_set_trajectory_type(trajectory_type_t type);

/** 设置微抖动幅度 (像素), 0 关闭 */
void autoxyq_set_jitter(uint8_t amplitude_px);

// ---- 异步控制 ----

/** 阻塞直到动作队列清空 */
void autoxyq_flush(void);

#endif // AUTOXYQ_H
