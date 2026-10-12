cmake_minimum_required(VERSION 3.20)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

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

# Shared debug/release flags, expressed as generator expressions so they can
# be folded directly into whichever options target needs them (kept as a
# CMake list variable, not a separate INTERFACE library, to avoid CMake
# mis-flattening empty option-only INTERFACE targets into literal -l flags).
set(QUARK_DEBUG_FLAGS
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-ggdb3>
        $<$<CONFIG:Debug>:-gdwarf-5>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Debug>:-fvar-tracking>
        $<$<CONFIG:Debug>:-fvar-tracking-assignments>
        $<$<CONFIG:Debug>:-fno-inline>
        $<$<CONFIG:Debug>:-fno-inline-functions>
)

set(QUARK_RELEASE_FLAGS
        $<$<CONFIG:Release>:-O3>
)

# Truly kernel-only: freestanding execution environment, no red zone
# (IST/interrupt safety), no PIC (fixed higher-half mapping), no builtins
# assumed. Not appropriate for hosted userspace binaries.
add_library(quark_kernel_freestanding_options INTERFACE)

target_compile_options(quark_kernel_freestanding_options INTERFACE
        -ffreestanding
        -fno-builtin
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -mno-red-zone
        -mno-mmx
        -mno-avx
        -mno-avx512f
)

# Language-level choices shared between kernel and userspace.
add_library(quark_lang_options INTERFACE)

target_compile_options(quark_lang_options INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

# Kernel options
add_library(quark_kernel_options INTERFACE)

target_link_libraries(quark_kernel_options INTERFACE
        quark_kernel_freestanding_options
        quark_lang_options
)

target_compile_options(quark_kernel_options INTERFACE
        -mcmodel=kernel
        -Wall
        -Wextra
        ${QUARK_DEBUG_FLAGS}
        ${QUARK_RELEASE_FLAGS}
)

# Userspace options — hosted, not freestanding. Non-PIC/non-PIE kept
# explicitly since the ELF loader currently assumes fixed-address,
# non-relocatable binaries; revisit if that changes.
add_library(quark_userspace_options INTERFACE)

target_link_libraries(quark_userspace_options INTERFACE
        quark_lang_options
)

target_compile_options(quark_userspace_options INTERFACE
        -mcmodel=small
        -fno-pic
        -fno-pie
        --sysroot=${QUARK_SYSROOT}
        ${QUARK_DEBUG_FLAGS}
        ${QUARK_RELEASE_FLAGS}
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
add_subdirectory(user/coreutil)

include(FetchContent)

FetchContent_Declare(
        limine
        URL https://github.com/limine-bootloader/limine/releases/download/v12.6.0/limine-binary.tar.xz
        URL_HASH SHA256=44fb480294463c15823bba5378c955b46d15d08fc7b5a74de49fef2953d1878d
)
FetchContent_MakeAvailable(limine)

add_custom_command(
        OUTPUT ${limine_SOURCE_DIR}/limine
        COMMAND make -C ${limine_SOURCE_DIR}
        DEPENDS ${limine_SOURCE_DIR}/limine.c
        COMMENT "Building limine deploy tool"
)
add_custom_target(limine_tool ALL DEPENDS ${limine_SOURCE_DIR}/limine)

set(ISO_DIR ${CMAKE_BINARY_DIR}/iso)
set(ISO_OUTPUT ${CMAKE_BINARY_DIR}/quark.iso)
set(LIMINE_SRC_DIR ${limine_SOURCE_DIR})
set(QUARK_IMG ${CMAKE_BINARY_DIR}/quark.img)

# Disk image
add_custom_command(
        OUTPUT ${QUARK_IMG}
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/build-img.sh --img ${QUARK_IMG}
        COMMENT "Creating FAT32 disk image"
        VERBATIM
)
add_custom_target(quark_img DEPENDS ${QUARK_IMG})

# Install into sysroot
add_custom_target(quark_sysroot_install
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
        DEPENDS ${KERNEL_TARGET} init snell quark_libc coreutils
        COMMENT "Installing kernel + userspace into sysroot"
        VERBATIM
)

# ISO
add_custom_target(quark_iso
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/build-iso.sh
        --iso-dir ${ISO_DIR}
        --sysroot ${QUARK_SYSROOT}
        --img ${QUARK_IMG}
        --limine-src ${LIMINE_SRC_DIR}
        --out ${ISO_OUTPUT}
        --limine-conf ${CMAKE_SOURCE_DIR}/limine.conf
        DEPENDS quark init snell ${CMAKE_SOURCE_DIR}/limine.conf limine_tool
        COMMENT "Creating bootable ISO"
        VERBATIM
)
add_dependencies(quark_iso quark_sysroot_install quark_img)