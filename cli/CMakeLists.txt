add_executable(autoxyq-cli
    autoxyq-cli.c
)

target_link_libraries(autoxyq-cli
    autoxyq-static
    setupapi.lib
)

target_include_directories(autoxyq-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)
