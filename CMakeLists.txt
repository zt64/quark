cmake_minimum_required(VERSION 3.20)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/toolchain/quark-toolchain.cmake")
endif ()

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
set(QUARK_TARGET x86_64-quark)
set(TOOLCHAIN_PREFIX "$ENV{HOME}/opt/cross" CACHE PATH "")
set(QUARK_SYSROOT "$ENV{HOME}/opt/quark-sysroot" CACHE PATH "")
set(CMAKE_INSTALL_PREFIX ${QUARK_SYSROOT} CACHE PATH "" FORCE)

add_library(quark_freestanding_options INTERFACE)

target_compile_options(quark_freestanding_options INTERFACE
        -ffreestanding
        -fno-builtin
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -mno-red-zone
        -mno-mmx
        -mno-avx
        -mno-avx512f

        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

add_library(quark_debug_options INTERFACE)

target_compile_options(quark_debug_options INTERFACE
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-ggdb3>
        $<$<CONFIG:Debug>:-gdwarf-5>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Debug>:-fvar-tracking>
        $<$<CONFIG:Debug>:-fvar-tracking-assignments>
        $<$<CONFIG:Debug>:-fno-inline>
        $<$<CONFIG:Debug>:-fno-inline-functions>
)

add_library(quark_release_options INTERFACE)

target_compile_options(quark_release_options INTERFACE
        $<$<CONFIG:Release>:-O3>
)

# Kernel options
add_library(quark_kernel_options INTERFACE)

target_link_libraries(quark_kernel_options INTERFACE
        quark_freestanding_options
        quark_debug_options
        quark_release_options
)

target_compile_options(quark_kernel_options INTERFACE
        -mcmodel=kernel
        -Wall
        -Wextra
)

# Userspace options
add_library(quark_userspace_options INTERFACE)

target_link_libraries(quark_userspace_options INTERFACE
        quark_freestanding_options
        quark_debug_options
        quark_release_options
)

target_compile_options(quark_userspace_options INTERFACE
        -mcmodel=small
        --sysroot=${QUARK_SYSROOT}
)

target_link_options(quark_userspace_options INTERFACE
        --sysroot=${QUARK_SYSROOT}
        -no-pie
)

# LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

include(ExternalProject)

add_subdirectory(libc)
add_subdirectory(kernel)
add_subdirectory(user/init)
add_subdirectory(user/snell)

find_program(XORRISO NAMES xorriso REQUIRED)
find_program(QEMU_SYSTEM_X86_64 NAMES qemu-system-x86_64 REQUIRED)
find_program(DD_TOOL NAMES dd REQUIRED)
find_program(MFORMAT_TOOL NAMES mformat REQUIRED)
find_program(MCOPY_TOOL NAMES mcopy REQUIRED)

set(ISO_DIR ${CMAKE_BINARY_DIR}/iso)
set(ISO_OUTPUT ${CMAKE_BINARY_DIR}/quark.iso)

set(LIMINE_SRC_DIR ${CMAKE_SOURCE_DIR}/external/limine)
set(LIMINE_DIR ${ISO_DIR}/boot/limine)
set(EFI_DIR ${ISO_DIR}/EFI/BOOT)

set(QUARK_IMG ${CMAKE_BINARY_DIR}/quark.img)

# Disk image
add_custom_command(
        OUTPUT ${QUARK_IMG}
        COMMAND ${DD_TOOL} if=/dev/zero of=${QUARK_IMG} bs=1M count=64 status=none
        COMMAND ${MFORMAT_TOOL} -i ${QUARK_IMG} -F ::
        COMMENT "Creating FAT32 disk image"
        VERBATIM
)

add_custom_target(quark_img DEPENDS ${QUARK_IMG})

# Install into sysroot
add_custom_target(quark_sysroot_install
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
        DEPENDS ${KERNEL_TARGET} init snell quark_libc
        COMMENT "Installing kernel + userspace into sysroot"
        VERBATIM
)

# ISO
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