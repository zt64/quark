file(GLOB_RECURSE SOURCES_ASM src/*.asm)
file(GLOB_RECURSE SOURCES_CXX src/*.cpp src/*.c)

add_executable(${KERNEL_TARGET} ${SOURCES_ASM} ${SOURCES_CXX})

target_include_directories(${KERNEL_TARGET} PRIVATE include)
target_link_libraries(${KERNEL_TARGET} PRIVATE quark_kernel_options quark_libc gcc)
target_link_options(${KERNEL_TARGET} PRIVATE -T ${CMAKE_SOURCE_DIR}/linker.ld -nostdlib -static -no-pie -mcmodel=kernel)

include(${CMAKE_SOURCE_DIR}/external/uacpi/uacpi.cmake)

target_compile_definitions(${KERNEL_TARGET} PRIVATE UACPI_BAREBONES_MODE)
target_sources(${KERNEL_TARGET} PRIVATE ${UACPI_SOURCES})
target_include_directories(${KERNEL_TARGET} PRIVATE ${UACPI_INCLUDES})

install(TARGETS ${KERNEL_TARGET}
        RUNTIME DESTINATION boot
)