#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "trajectory.h"
#include "delay.h"

// 外部变量 — 来自 autoxyq.c 或全局配置
extern uint8_t g_JitterAmplitude;  // 微抖动幅度
extern uint32_t g_RandomSeed;

// 三点线性插值
static double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// 二次贝塞尔: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
static double bezier2(double p0, double p1, double p2, double t) {
    double u = 1.0 - t;
    return u * u * p0 + 2.0 * u * t * p1 + t * t * p2;
}

// 三次贝塞尔: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
static double bezier3(double p0, double p1, double p2, double p3, double t) {
    double u = 1.0 - t;
    return u * u * u * p0 + 3.0 * u * u * t * p1 +
           3.0 * u * t * t * p2 + t * t * t * p3;
}

// 速度曲线建模 — 加速 → 匀速 → 减速
// 返回规范化后的时间映射 t_norm → t_warped
static double speed_profile(double t, double accel_ratio, double decel_ratio) {
    // accel_ratio: 加速段占比 (0.0~0.3)
    // decel_ratio: 减速段占比 (0.0~0.3)
    // 中间段: 匀速

    if (t < accel_ratio) {
        // 加速段: 0 → 1 平方加速 (ease-in)
        double p = t / accel_ratio;
        return accel_ratio * (p * p) / (accel_ratio + (1.0 - accel_ratio - decel_ratio) + decel_ratio * 0.5);
    } else if (t > 1.0 - decel_ratio) {
        // 减速段: ease-out
        double p = (t - (1.0 - decel_ratio)) / decel_ratio;
        // Fitts' Law: 越接近终点越慢
        double fitts_factor = 1.0 - (1.0 - p) * (1.0 - p) * 0.3;
        double raw = (1.0 - (1.0 - p) * (1.0 - p)) * fitts_factor;
        return 1.0 - decel_ratio * (1.0 - raw);
    } else {
        // 匀速段
        double mid_start = accel_ratio;
        double mid_duration = 1.0 - accel_ratio - decel_ratio;
        double p = (t - mid_start) / mid_duration;
        return accel_ratio + p * mid_duration;
    }
}

// 均匀随机噪声 [-amplitude, +amplitude]
static int16_t random_jitter(int16_t amplitude) {
    if (amplitude <= 0) return 0;
    int range = (int)amplitude * 2 + 1;
    return (int16_t)((rand() % range) - amplitude);
}

// 生成路径点
int trajectory_generate_path(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              uint32_t duration_ms,
                              trajectory_type_t type,
                              int16_t** out_path,
                              uint32_t* out_count) {
    if (out_path == NULL || out_count == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }
    if (duration_ms == 0) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    // 帧率: ~125Hz (匹配 USB HID 默认 polling rate)
    #define HID_FRAME_INTERVAL_MS 8
    uint32_t frame_count = (duration_ms + HID_FRAME_INTERVAL_MS - 1) / HID_FRAME_INTERVAL_MS;
    if (frame_count < 2) frame_count = 2;
    if (frame_count > 10000) frame_count = 10000; // 安全上限

    int16_t* path = (int16_t*)malloc(frame_count * 2 * sizeof(int16_t));
    if (path == NULL) {
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    // 随机控制点偏移量 (路径长度的 10%-30%)
    double total_dist = sqrt((double)(x1 - x0) * (x1 - x0) + (double)(y1 - y0) * (y1 - y0));
    double cp_offset = total_dist * (0.1 + (double)rand() / RAND_MAX * 0.2);

    // 随机角度
    double cp_angle = ((double)rand() / RAND_MAX) * 2.0 * 3.1415926535;
    int16_t cp1_dx = (int16_t)(cos(cp_angle) * cp_offset);
    int16_t cp1_dy = (int16_t)(sin(cp_angle) * cp_offset);

    cp_angle = ((double)rand() / RAND_MAX) * 2.0 * 3.1415926535;
    int16_t cp2_dx = (int16_t)(cos(cp_angle) * cp_offset);
    int16_t cp2_dy = (int16_t)(sin(cp_angle) * cp_offset);

    // 加速/减速比 (随机化)
    double accel_ratio = 0.15 + ((double)rand() / RAND_MAX) * 0.10; // 15%-25%
    double decel_ratio = 0.20 + ((double)rand() / RAND_MAX) * 0.10; // 20%-30% (减速比加速长)

    // 生成每帧坐标
    for (uint32_t i = 0; i < frame_count; i++) {
        double t_raw = (double)i / (double)(frame_count - 1);
        double t_speed = speed_profile(t_raw, accel_ratio, decel_ratio);

        double px, py;
        switch (type) {
            case TRAJECTORY_LINEAR:
                px = lerp(x0, x1, t_speed);
                py = lerp(y0, y1, t_speed);
                break;

            case TRAJECTORY_BEZIER2:
                px = bezier2(x0, x0 + cp1_dx, x1, t_speed);
                py = bezier2(y0, y0 + cp1_dy, y1, t_speed);
                break;

            case TRAJECTORY_BEZIER3:
            default:
                px = bezier3(x0, x0 + cp1_dx, x1 + cp2_dx, x1, t_speed);
                py = bezier3(y0, y0 + cp1_dy, y1 + cp2_dy, y1, t_speed);
                break;
        }

        // Fitts' Law 修正: 接近终点时降速
        double remaining_ratio = 1.0 - t_raw;
        if (remaining_ratio < 0.15) {
            double fitts_scale = 0.5 + 0.5 * (remaining_ratio / 0.15);
            double px_end = (type == TRAJECTORY_LINEAR) ?
                lerp(x0, x1, t_speed) : bezier3(x0, x0 + cp1_dx, x1 + cp2_dx, x1, t_speed);
            double py_end = (type == TRAJECTORY_LINEAR) ?
                lerp(y0, y1, t_speed) : bezier3(y0, y0 + cp1_dy, y1 + cp2_dy, y1, t_speed);
            px = px_end * (1.0 - fitts_scale) + x1 * fitts_scale;
            py = py_end * (1.0 - fitts_scale) + y1 * fitts_scale;
        }

        // 微抖动
        int16_t jitter_x = random_jitter(2);
        int16_t jitter_y = random_jitter(2);

        path[i * 2]     = (int16_t)px + jitter_x;
        path[i * 2 + 1] = (int16_t)py + jitter_y;
    }

    // 确保终点精确到达
    path[(frame_count - 1) * 2]     = x1;
    path[(frame_count - 1) * 2 + 1] = y1;

    *out_path = path;
    *out_count = frame_count;
    return AUTOXYQ_OK;
}
