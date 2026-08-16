cmake_minimum_required(VERSION 3.20)
project(quark LANGUAGES C CXX ASM_NASM)

set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(KERNEL_TARGET quark)

add_library(quark_freestanding_options INTERFACE)
target_compile_options(quark_freestanding_options INTERFACE
        -fno-builtin
        -fno-stack-protector
        -mno-red-zone
        -mno-mmx
        -mno-avx
        -mno-avx512f
        -fno-pic
        -fno-pie
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

add_library(quark_kernel_options INTERFACE)
target_link_libraries(quark_kernel_options INTERFACE quark_freestanding_options)
target_compile_options(quark_kernel_options INTERFACE
        -mcmodel=kernel
        -Wall
        -Wextra
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O3>
)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

add_library(quark_userspace_options INTERFACE)
target_link_libraries(quark_userspace_options INTERFACE quark_freestanding_options)
target_compile_options(quark_userspace_options INTERFACE
        -mcmodel=small
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O3>
)
target_link_options(quark_userspace_options INTERFACE
        -nostdlib
        -static
        -no-pie
        -Wl,-T,${CMAKE_SOURCE_DIR}/user/linker.ld
)
# Userspace executables build with -nostdlib and their own entry point, so
# GCC's crtbegin.o/crtend.o (which would normally define __dso_handle) are
# never linked in. Provide it centrally here so every target that links
# quark_userspace_options gets it, instead of duplicating it per program.
# TODO: Figure out if this is actually needed
target_sources(quark_userspace_options INTERFACE ${CMAKE_SOURCE_DIR}/user/crt/dso_handle.cpp)

set(TOOLCHAIN_PREFIX ${CMAKE_BINARY_DIR}/toolchain)
set(QUARK_SYSROOT ${CMAKE_BINARY_DIR}/sysroot CACHE PATH "Quark sysroot root")
set(CMAKE_INSTALL_PREFIX ${QUARK_SYSROOT} CACHE PATH "" FORCE)

set(GCC_VERSION 15.3.0)

include(ExternalProject)

include(toolchain/headers.cmake)   # defines mlibc_headers, target now exists in this scope
add_subdirectory(libc)             # mlibc_build can DEPENDS gcc_build
add_subdirectory(toolchain)        # gcc_build can DEPENDS mlibc_headers, target already known

# mlibc_build and gcc_bootstrap each need to reference the other's target name
# (mlibc_build needs the bootstrap compiler binary; gcc_final needs mlibc's
# libc.a)
add_dependencies(mlibc_build gcc_bootstrap)

add_subdirectory(kernel)
add_subdirectory(user/init)
add_subdirectory(user/snell)

find_program(XORRISO NAMES xorriso)
if (NOT XORRISO)
    message(FATAL_ERROR "xorriso not found")
endif ()

find_program(QEMU_SYSTEM_X86_64 NAMES qemu-system-x86_64)
if (NOT QEMU_SYSTEM_X86_64)
    message(FATAL_ERROR "qemu-system-x86_64 not found")
endif ()

set(ISO_DIR ${CMAKE_BINARY_DIR}/iso)
set(ISO_OUTPUT ${CMAKE_BINARY_DIR}/quark.iso)
set(LIMINE_SRC_DIR ${CMAKE_SOURCE_DIR}/external/limine)
set(LIMINE_DIR ${ISO_DIR}/boot/limine)
set(QEMU_COMMON_ARGS
        -cdrom ${ISO_OUTPUT}
        -net none
        -serial mon:stdio
        -m 4G
        -audiodev pa,id=speaker
        -machine pcspk-audiodev=speaker
        -no-reboot
        -no-shutdown
        -smbios type=0,vendor=hog_vendor,version=1.2.3
        -smbios type=3,version=te
)

find_program(DD_TOOL NAMES dd REQUIRED)
find_program(MFORMAT_TOOL NAMES mformat REQUIRED)
find_program(MCOPY_TOOL NAMES mcopy REQUIRED)
find_program(MMD_TOOL NAMES mmd REQUIRED)

set(QUARK_IMG ${CMAKE_CURRENT_BINARY_DIR}/quark.img)

add_custom_command(
        OUTPUT ${QUARK_IMG}
        COMMAND ${DD_TOOL} if=/dev/zero of=${QUARK_IMG} bs=1M count=64 status=none
        COMMAND ${MFORMAT_TOOL} -i ${QUARK_IMG} -F ::
        COMMENT "Creating FAT32 disk image"
        VERBATIM
)
add_custom_target(quark_img DEPENDS ${QUARK_IMG})

add_custom_target(quark_sysroot_install
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
        DEPENDS ${KERNEL_TARGET} init snell quark_libc
        COMMENT "Installing kernel + userspace into sysroot"
        VERBATIM
)

add_custom_target(quark_iso
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${ISO_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LIMINE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${QUARK_SYSROOT}/boot/quark ${ISO_DIR}/boot/quark
        COMMAND ${CMAKE_COMMAND} -E copy ${QUARK_SYSROOT}/boot/init ${ISO_DIR}/boot/init.elf
        COMMAND ${CMAKE_COMMAND} -E copy ${QUARK_SYSROOT}/boot/snell ${ISO_DIR}/boot/snell.elf
        COMMAND ${MCOPY_TOOL} -i ${QUARK_IMG} -o -s ${QUARK_SYSROOT}/boot ::/

        COMMAND ${CMAKE_COMMAND} -E copy ${QUARK_IMG} ${ISO_DIR}/quark.img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine.conf ${LIMINE_SRC_DIR}/limine-bios.sys
        ${LIMINE_SRC_DIR}/limine-bios-cd.bin ${LIMINE_SRC_DIR}/limine-uefi-cd.bin ${LIMINE_DIR}

        COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_DIR}/EFI/BOOT
        COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_SRC_DIR}/BOOTX64.EFI ${ISO_DIR}/EFI/BOOT
        COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_SRC_DIR}/BOOTIA32.EFI ${ISO_DIR}/EFI/BOOT

        COMMAND ${XORRISO} -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin
        -efi-boot-part --efi-boot-image --protective-msdos-label
        ${ISO_DIR} -o ${ISO_OUTPUT}

        COMMAND ${LIMINE_SRC_DIR}/limine bios-install ${ISO_OUTPUT}
        COMMENT "Creating bootable ISO"
        VERBATIM
)
add_dependencies(quark_iso quark_sysroot_install quark_img)