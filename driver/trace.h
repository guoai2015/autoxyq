#ifndef TRACE_H
#define TRACE_H

#include <windows.h>

// 轻量调试输出 — 使用 OutputDebugStringA, 可通过 DbgView 查看
// 驱动端不使用 printf (UMDF 运行在 WUDFHost.exe 中, 无控制台)

#ifdef _DEBUG
#define TRACE(fmt, ...) do { \
    char _buf[256]; \
    _snprintf_s(_buf, sizeof(_buf), _TRUNCATE, \
        "[autoxyq-drv] " fmt "\n", ##__VA_ARGS__); \
    OutputDebugStringA(_buf); \
} while(0)
#else
#define TRACE(fmt, ...) ((void)0)
#endif

#endif // TRACE_H
