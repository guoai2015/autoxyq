# 驱动源文件
set(DRIVER_SOURCES
    autoxyq-drv.c
    hid-keyboard.c
    hid-mouse.c
    ioctl-handler.c
)

# UMDF 驱动编译为 DLL
add_library(autoxyq-drv SHARED ${DRIVER_SOURCES})

# 设置 UMDF 驱动属性
set_target_properties(autoxyq-drv PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
)

# WDK include 路径由 setup-wdk action 自动配置
# 本地需确保 WDK 环境变量已设置

# 链接 WDF 用户态库
target_link_libraries(autoxyq-drv
    WdfUmDriverStub.lib
)

# 驱动专用编译定义
target_compile_definitions(autoxyq-drv PRIVATE
    _UMDF_
    UMDF2_USE_VERSION=2
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
