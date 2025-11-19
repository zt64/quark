if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(QUARK_TARGET x86_64-quark)

include(ExternalProject)

find_program(GIT_EXE NAMES git REQUIRED)

set(QUARK_DL_CACHE ${CMAKE_SOURCE_DIR}/.cache/downloads)
file(MAKE_DIRECTORY ${QUARK_DL_CACHE})

ExternalProject_Add(binutils_build
        URL https://ftpmirror.gnu.org/binutils/binutils-2.45.1.tar.xz
        URL_HASH SHA256=5fe101e6fe9d18fdec95962d81ed670fdee5f37e3f48f0bef87bddf862513aa5
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        PATCH_COMMAND ${GIT_EXE} apply --unsafe-paths -p1
        ${CMAKE_SOURCE_DIR}/toolchain/patches/binutils-quark.patch
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET} --prefix=/usr
        --with-sysroot=${QUARK_SYSROOT}
        --disable-werror --enable-default-execstack=no
        BUILD_COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND make install
        BUILD_BYPRODUCTS ${TOOLCHAIN_PREFIX}/bin/${QUARK_TARGET}-ld
)

ExternalProject_Add(gcc_build
        DEPENDS binutils_build
        URL https://ftpmirror.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        PATCH_COMMAND ${GIT_EXE} apply --unsafe-paths -p1
        --directory=<SOURCE_DIR>
        ${CMAKE_SOURCE_DIR}/toolchain/patches/gcc-quark.patch
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET} --prefix=/usr
        --with-sysroot=${QUARK_SYSROOT}
        --enable-languages=c,c++ --disable-multilib
        --enable-shared
        --enable-host-shared
        BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        make all-gcc all-target-libgcc -j8
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        make install-gcc install-target-libgcc
        BUILD_BYPRODUCTS ${TOOLCHAIN_PREFIX}/bin/${QUARK_TARGET}-gcc
)
#set(CMAKE_C_COMPILER_WORKS 1)
#set(CMAKE_CXX_COMPILER_WORKS 1)
## Don't try to link during `try_compile()`, just in case.
#set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
#
## Look for host programs in the host environment, not the target.
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#
## Do look for libraries and includes in the target sysroot.
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)