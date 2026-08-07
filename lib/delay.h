#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

// 设置随机延迟范围 (毫秒)
void delay_set_range(uint32_t min_ms, uint32_t max_ms);

// 返回 [min_ms, max_ms] 范围内均匀随机值 (毫秒)
uint32_t delay_random_ms(void);

// 微秒级高精度睡眠
void delay_sleep_us(uint64_t us);

#endif // DELAY_H
