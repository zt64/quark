add_executable(snell
        src/start.asm
        src/main.cpp
)

target_link_libraries(snell PRIVATE quark_userspace_options)

target_link_options(snell PRIVATE
        -nostdlib
        -static
        -no-pie
        -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

target_link_libraries(snell PRIVATE quark_libc)
install(TARGETS snell
        RUNTIME DESTINATION boot
)