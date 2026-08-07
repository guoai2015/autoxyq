#include <windows.h>
#include <stdlib.h>
#include "delay.h"

static uint32_t g_DelayMinMs = 10;
static uint32_t g_DelayMaxMs = 50;

void delay_set_range(uint32_t min_ms, uint32_t max_ms) {
    g_DelayMinMs = min_ms;
    g_DelayMaxMs = max_ms;
}

uint32_t delay_random_ms(void) {
    if (g_DelayMinMs >= g_DelayMaxMs) {
        return g_DelayMinMs;
    }

    // 使用 rand() 的均匀分布
    uint32_t range = g_DelayMaxMs - g_DelayMinMs;
    uint32_t random_offset = (uint32_t)rand() % (range + 1);
    return g_DelayMinMs + random_offset;
}

void delay_sleep_us(uint64_t us) {
    if (us == 0) return;

    // 使用 QueryPerformanceCounter 实现高精度忙等待
    // 对于亚毫秒级延迟使用自旋，毫秒级以上使用 Sleep
    if (us >= 1000) {
        // 毫秒级部分使用 Sleep
        DWORD ms = (DWORD)(us / 1000);
        Sleep(ms);
        us = us % 1000;
    }

    if (us == 0) return;

    // 微秒级部分使用性能计数器自旋
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        // 回退到 Sleep(1)
        Sleep(1);
        return;
    }

    LARGE_INTEGER start, current;
    QueryPerformanceCounter(&start);

    // 计算目标计数值: us * freq / 1_000_000
    uint64_t target_ticks = (us * freq.QuadPart) / 1000000ULL;

    do {
        QueryPerformanceCounter(&current);
    } while ((uint64_t)(current.QuadPart - start.QuadPart) < target_ticks);
}
