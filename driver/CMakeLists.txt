# 驱动源文件
set(DRIVER_SOURCES
    autoxyq-drv.c
    hid-keyboard.c
    hid-mouse.c
    ioctl-handler.c
)

# UMDF 驱动编译为 DLL
add_library(autoxyq-drv SHARED ${DRIVER_SOURCES})

set_target_properties(autoxyq-drv PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
)

# 查找 WDK 安装路径
file(GLOB WDK_KIT_DIR "C:/Program Files (x86)/Windows Kits/10/Include/10.*")
list(SORT WDK_KIT_DIR)
list(REVERSE WDK_KIT_DIR)
list(GET WDK_KIT_DIR 0 WDK_INCLUDE_DIR)

# UMDF 是用户态驱动, 需要 um (用户态) + shared (共享) 头文件, 不需要 km (内核态)
set(WDK_UM_INCLUDE  "${WDK_INCLUDE_DIR}/um")
set(WDK_SHARED_INCLUDE "${WDK_INCLUDE_DIR}/shared")

# WDF include 路径 (UMDF 2.0 或 2.x)
file(GLOB WDK_WDF_INCLUDE_LIST "C:/Program Files (x86)/Windows Kits/10/Include/wdf/umdf/*")
list(SORT WDK_WDF_INCLUDE_LIST)
list(REVERSE WDK_WDF_INCLUDE_LIST)
if(WDK_WDF_INCLUDE_LIST)
    list(GET WDK_WDF_INCLUDE_LIST 0 WDK_WDF_INCLUDE)
else()
    set(WDK_WDF_INCLUDE "C:/Program Files (x86)/Windows Kits/10/Include/wdf/umdf/2.0")
    message(WARNING "WDF include 目录未找到, 使用默认路径 ${WDK_WDF_INCLUDE}")
endif()

# 查找 WDK 库路径
file(GLOB WDK_LIB_DIR "C:/Program Files (x86)/Windows Kits/10/Lib/10.*")
list(SORT WDK_LIB_DIR)
list(REVERSE WDK_LIB_DIR)
list(GET WDK_LIB_DIR 0 WDK_LIB_BASE)
set(WDK_UM_LIB "${WDK_LIB_BASE}/um/x64")
set(WDK_WDF_LIB "C:/Program Files (x86)/Windows Kits/10/Lib/wdf/umdf/x64")

target_include_directories(autoxyq-drv PRIVATE
    ${WDK_UM_INCLUDE}
    ${WDK_SHARED_INCLUDE}
    ${WDK_WDF_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# WDF 头文件内部有结构体对齐警告 (C4324), 属 WDK 自身问题, 抑制之
target_compile_options(autoxyq-drv PRIVATE /wd4324)

target_link_directories(autoxyq-drv PRIVATE
    ${WDK_UM_LIB}
    ${WDK_WDF_LIB}
)

target_link_libraries(autoxyq-drv
    WdfUmDriverStub.lib
    onecoreuap.lib
)

# 驱动专用编译定义
target_compile_definitions(autoxyq-drv PRIVATE
    _UMDF_
    UMDF2_USE_VERSION=2
    WIN32_NO_STATUS
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
