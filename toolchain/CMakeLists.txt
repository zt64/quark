if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

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
        # Without this, ExternalProject re-runs its "update" check on every
        # single build invocation (even for plain URL sources with nothing to
        # update), which touches a stamp file and cascades into needless
        # re-patch/re-configure/re-build of the whole project every time you
        # build ANY target, even ones unrelated to binutils.
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${CMAKE_SOURCE_DIR}/toolchain/patches/binutils-quark.patch
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET} --prefix=${TOOLCHAIN_PREFIX}
        --with-sysroot=${QUARK_SYSROOT}
        --disable-werror --enable-default-execstack=no
        BUILD_COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND make install
        BUILD_BYPRODUCTS ${TOOLCHAIN_PREFIX}/bin/${QUARK_TARGET}-ld
)

set(QUARK_TARGET x86_64-quark)
set(QUARK_DL_CACHE ${CMAKE_SOURCE_DIR}/.cache/downloads)

# ==============================================================================
# STEP 1: GCC STAGE 1 (Bootstrap C Compiler Only)
# ==============================================================================
# This step does NOT build target libraries like libstdc++. It only builds the
# bare compiler and libgcc so that Meson can use it to build mlibc.
# ==============================================================================
#url https://free.nchc.org.tw/gcc/releases/gcc-15.3.0/gcc-15.3.0.tar.xz
ExternalProject_Add(gcc_bootstrap
        DEPENDS binutils_build mlibc_headers
        URL https://free.nchc.org.tw/gcc/releases/gcc-15.3.0/gcc-15.3.0.tar.xz
        URL_HASH SHA256=fa59c1beef8995f27c4d71c1df227587189315d3e6faff1bb4306e61b0c530eb
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        # See binutils_build's comment: prevents a needless "update" check
        # (and the full re-patch/re-configure/re-build cascade it triggers)
        # from running on every single build invocation.
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${CMAKE_SOURCE_DIR}/toolchain/patches/gcc-quark.patch

        CONFIGURE_COMMAND <SOURCE_DIR>/configure
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

        # Stage 1 must stay compiler-only: libstdc++ is built later, once the
        # target libc headers are installed in the sysroot. Building it here with
        # --without-headers leaves the C++ compatibility wrappers (e.g. cwchar)
        # half-installed and breaks every subsequent userspace build.
        BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make all-gcc all-target-libgcc -j8"

        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make install-gcc install-target-libgcc"
)


# ==============================================================================
# STEP 3: GCC STAGE 2 (Final C/C++ Compiler & Libstdc++)
# ==============================================================================
# This step depends on mlibc_build. Because libc.a is now present in the
# sysroot, the libstdc++-v3 configure script will successfully pass link tests.
# ==============================================================================
ExternalProject_Add(gcc_final
        DEPENDS mlibc_build
        URL https://free.nchc.org.tw/gcc/releases/gcc-15.3.0/gcc-15.3.0.tar.xz
        URL_HASH SHA256=fa59c1beef8995f27c4d71c1df227587189315d3e6faff1bb4306e61b0c530eb
        DOWNLOAD_DIR ${QUARK_DL_CACHE}
        DOWNLOAD_NO_PROGRESS TRUE
        # See binutils_build's comment above.
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND patch -p1 --forward < ${CMAKE_SOURCE_DIR}/toolchain/patches/gcc-quark.patch

        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --target=${QUARK_TARGET}
        --prefix=${TOOLCHAIN_PREFIX}
        --with-sysroot=${QUARK_SYSROOT}
        --enable-languages=c,c++
        --disable-multilib
        --disable-nls
        --disable-shared
        --disable-threads

        # Final stage has a usable target libc in the sysroot, so libstdc++ can
        # configure and install the C++ compatibility wrappers such as cwchar.
        BUILD_COMMAND
        ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make -j8"

        INSTALL_COMMAND
        ${CMAKE_COMMAND} -E env PATH=${TOOLCHAIN_PREFIX}/bin:$ENV{PATH}
        sh -c "make install-gcc install-target-libgcc install-target-libstdc++-v3"
)