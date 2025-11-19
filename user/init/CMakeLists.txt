add_executable(init
        src/start.asm
        src/main.cpp
)

target_link_libraries(init PRIVATE quark_userspace_options)

target_link_options(init PRIVATE
        -nostdlib
        -static
        -no-pie
        -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

target_link_libraries(init PRIVATE quark_libc)
install(TARGETS init
        RUNTIME DESTINATION boot
)