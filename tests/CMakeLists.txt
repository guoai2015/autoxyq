enable_testing()

# test-delay
add_executable(test-delay
    test-delay.c
    ${CMAKE_SOURCE_DIR}/lib/delay.c
)
target_include_directories(test-delay PRIVATE ${CMAKE_SOURCE_DIR}/lib)
add_test(NAME test-delay COMMAND test-delay)

# test-trajectory
add_executable(test-trajectory
    test-trajectory.c
    ${CMAKE_SOURCE_DIR}/lib/trajectory.c
    ${CMAKE_SOURCE_DIR}/lib/delay.c
)
target_include_directories(test-trajectory PRIVATE ${CMAKE_SOURCE_DIR}/lib)
add_test(NAME test-trajectory COMMAND test-trajectory)

# test-queue
# 说明: action-queue.c 的工作线程会调用 autoxyq_* 函数 (autoxyq.h 中声明),
# 因此除 action-queue.c 外还必须链接 autoxyq.c 及其依赖
# (ioctl-client.c / trajectory.c / delay.c), 否则无法解析符号。
# trajectory.c 以 extern 引用 g_JitterAmplitude / g_RandomSeed,
# 而 lib/autoxyq.c 中二者为 static, 故额外生成外部可见的定义提供链接。
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test-queue-globals.c"
"#include <stdint.h>\n"
"uint8_t g_JitterAmplitude = 0;\n"
"uint32_t g_RandomSeed = 12345;\n")

add_executable(test-queue
    test-queue.c
    ${CMAKE_SOURCE_DIR}/lib/action-queue.c
    ${CMAKE_SOURCE_DIR}/lib/autoxyq.c
    ${CMAKE_SOURCE_DIR}/lib/ioctl-client.c
    ${CMAKE_SOURCE_DIR}/lib/trajectory.c
    ${CMAKE_SOURCE_DIR}/lib/delay.c
    ${CMAKE_CURRENT_BINARY_DIR}/test-queue-globals.c
)
target_include_directories(test-queue PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/driver
)
target_compile_definitions(test-queue PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
target_link_libraries(test-queue PRIVATE setupapi.lib)
add_test(NAME test-queue COMMAND test-queue)
