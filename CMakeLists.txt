cmake_minimum_required(VERSION 3.21)
project(autoxyq VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /wd4100 /wd4201 /wd4214)
endif()

enable_testing()

add_subdirectory(driver)
add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(tests)
