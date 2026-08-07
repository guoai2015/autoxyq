cmake_minimum_required(VERSION 3.21)
project(autoxyq VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 全局编译选项
if(MSVC)
    add_compile_options(/W4 /WX /wd4100 /wd4201 /wd4214)
endif()

# 子目录将在后续任务中逐步启用
add_subdirectory(driver)
# add_subdirectory(lib)
# add_subdirectory(cli)
# add_subdirectory(tests)
