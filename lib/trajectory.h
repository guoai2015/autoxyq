#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <stdint.h>
#include "autoxyq.h"
#include "autoxyq-error.h"

// 生成路径点数组
// x0, y0: 起点 (相对坐标)
// x1, y1: 终点 (相对坐标)
// duration_ms: 移动总时长
// type: 轨迹类型
// out_path: 输出路径点数组 (调用方负责 free), 格式 [x0, y0, x1, y1, ...]
// out_count: 输出路径点数量
// 返回 AUTOXYQ_OK 成功
int trajectory_generate_path(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              uint32_t duration_ms,
                              trajectory_type_t type,
                              int16_t** out_path,
                              uint32_t* out_count);

#endif // TRAJECTORY_H
