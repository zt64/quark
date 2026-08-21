cmake_minimum_required(VERSION 3.20)
project(quark-toolchain LANGUAGES NONE)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

include(ExternalProject)

# Repository root
get_filename_component(QUARK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(GCC_VERSION "15.3.0" CACHE STRING "GCC version") # Injected into quark-cross.ini.in
set(QUARK_TARGET x86_64-quark)

# Install locations
set(TOOLCHAIN_PREFIX "$ENV{HOME}/opt/cross")
set(QUARK_SYSROOT "$ENV{HOME}/opt/quark-sysroot")

set(QUARK_DL_CACHE "${QUARK_ROOT}/.cache/downloads")
file(MAKE_DIRECTORY "${QUARK_DL_CACHE}")

find_program(MESON_EXE meson REQUIRED)
find_program(NINJA_EXE ninja REQUIRED)

configure_file(
        "${QUARK_ROOT}/toolchain/quark-cross.ini.in"
        "${CMAKE_BINARY_DIR}/quark-cross.ini"
        @ONLY
)

#
# mlibc headers
#
ExternalProject_Add(mlibc_headers
        SOURCE_DIR "${QUARK_ROOT}/libc/mlibc"

        CONFIGURE_COMMAND ${MESON_EXE} setup
        --cross-file ${CMAKE_BINARY_DIR}/quark-cross.ini
        --prefix=/usr
        -Dheaders_only=true
        <BINARY_DIR> <SOURCE_DIR>

        BUILD_COMMAND ""

        INSTALL_COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${QUARK_SYSROOT} ${NINJA_EXE} -C <BINARY_DIR> install
)

#
# Binutils
#
ExternalProject_Add(binutils_build
        URL https://ftp.gnu.org/gnu/binutils/binutils-2.47.tar.xz
        URL_HASH SHA256=154ab23b60070e8f27013c22977f1129425d67d1e8acd6e13010e617811e4cff
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${QUARK_ROOT}/toolchain/patches/binutils-quark.patch
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET}
        --prefix=${TOOLCHAIN_PREFIX}
        --with-sysroot=${QUARK_SYSROOT}
        --disable-werror
        --enable-default-execstack=no
        BUILD_COMMAND make -j14
        INSTALL_COMMAND make install
        BUILD_BYPRODUCTS ${TOOLCHAIN_PREFIX}/bin/${QUARK_TARGET}-ld
)

#
# GCC Stage 1
#
ExternalProject_Add(gcc_bootstrap
        DEPENDS binutils_build mlibc_headers
        URL https://ftp.gnu.org/gnu/gcc/gcc-15.3.0/gcc-15.3.0.tar.xz
        URL_HASH SHA256=fa59c1beef8995f27c4d71c1df227587189315d3e6faff1bb4306e61b0c530eb
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${QUARK_ROOT}/toolchain/patches/gcc-quark.patch

        CONFIGURE_COMMAND
        <SOURCE_DIR>/configure
        --target=${QUARK_TARGET}
        --prefix=${TOOLCHAIN_PREFIX}
        --with-sysroot=${QUARK_SYSROOT}
        --enable-languages=c,c++
        --disable-multilib
        --disable-nls
        --disable-shared
        --disable-threads
        --disable-hosted-libstdcxx
        --without-headers

        BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make all-gcc all-target-libgcc -j14"

        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make install-gcc install-target-libgcc"
)

#
# mlibc
#
ExternalProject_Add(mlibc_build
        SOURCE_DIR "${QUARK_ROOT}/libc/mlibc"
        DEPENDS mlibc_headers gcc_bootstrap
        CONFIGURE_COMMAND ${MESON_EXE} setup
        --cross-file ${CMAKE_BINARY_DIR}/quark-cross.ini
        --prefix=/usr -Ddefault_library=static -Dno_headers=true
        -Duse_freestnd_hdrs=enabled
        <BINARY_DIR> <SOURCE_DIR>
        BUILD_COMMAND ${NINJA_EXE} -C <BINARY_DIR>
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${QUARK_SYSROOT} ${NINJA_EXE} -C <BINARY_DIR> install
        BUILD_BYPRODUCTS ${QUARK_SYSROOT}/usr/lib/libc.a
)

#
# GCC Stage 2
#
ExternalProject_Add(gcc_final
        DEPENDS mlibc_build
        URL https://free.nchc.org.tw/gcc/releases/gcc-15.3.0/gcc-15.3.0.tar.xz
        URL_HASH SHA256=fa59c1beef8995f27c4d71c1df227587189315d3e6faff1bb4306e61b0c530eb
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${QUARK_ROOT}/toolchain/patches/gcc-quark.patch
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET}
        --prefix=${TOOLCHAIN_PREFIX}
        --with-sysroot=${QUARK_SYSROOT}
        --with-native-system-header-dir=/usr/include
        --enable-languages=c,c++
        --disable-multilib
        --disable-nls
        --enable-shared
        --enable-host-shared
        BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH} sh -c "make all-gcc all-target-libgcc all-target-libstdc++-v3 -j14"
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make install-gcc install-target-libgcc install-target-libstdc++-v3"
)

add_custom_target(toolchain ALL DEPENDS gcc_final)