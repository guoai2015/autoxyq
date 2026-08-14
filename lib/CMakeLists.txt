set(LIB_SOURCES
    autoxyq.c
    ioctl-client.c
    delay.c
    trajectory.c
    action-queue.c
)

# 动态库
add_library(autoxyq SHARED ${LIB_SOURCES})

target_compile_definitions(autoxyq PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A000003
)

target_include_directories(autoxyq PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/driver
)

# SendInput 后端无需 setupapi (SendInput 在 user32.lib, MSVC 默认链接)

# 同时生成静态库供 CLI 测试使用
add_library(autoxyq-static STATIC ${LIB_SOURCES})

target_compile_definitions(autoxyq-static PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A000003
)

target_include_directories(autoxyq-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/driver
)
