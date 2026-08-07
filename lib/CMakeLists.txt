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
)

target_include_directories(autoxyq PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(autoxyq
    setupapi.lib
)

# 同时生成静态库供 CLI 测试使用
add_library(autoxyq-static STATIC ${LIB_SOURCES})

target_compile_definitions(autoxyq-static PRIVATE
    _UNICODE
    UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

target_include_directories(autoxyq-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
