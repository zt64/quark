# libc/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(libc LANGUAGES C CXX)

# --- Freestanding kernel-side libc (existing hand-written code) ---
file(GLOB_RECURSE LIBC_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
add_library(quark_kernel_libc STATIC ${LIBC_SOURCES})
set_property(TARGET quark_kernel_libc PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE FALSE)
target_include_directories(quark_kernel_libc
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/kernel/include
)
target_link_libraries(quark_kernel_libc PRIVATE quark_kernel_options)

# --- Hosted userspace libc (mlibc, built via Meson into the shared sysroot) ---
include(ExternalProject)

find_program(MESON_EXE NAMES meson REQUIRED)
find_program(NINJA_EXE NAMES ninja REQUIRED)

configure_file(
        ${CMAKE_SOURCE_DIR}/toolchain/quark-cross.ini.in
        ${CMAKE_BINARY_DIR}/quark-cross.ini
        @ONLY
)

ExternalProject_Add(mlibc_headers
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mlibc
        CONFIGURE_COMMAND ${MESON_EXE} setup
        --cross-file ${CMAKE_BINARY_DIR}/quark-cross.ini
        --prefix=/usr -Dheaders_only=true
        <BINARY_DIR> <SOURCE_DIR>
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${QUARK_SYSROOT} ${NINJA_EXE} -C <BINARY_DIR> install
)

ExternalProject_Add(mlibc_build
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mlibc
        DEPENDS mlibc_headers
        CONFIGURE_COMMAND ${MESON_EXE} setup
        --cross-file ${CMAKE_BINARY_DIR}/quark-cross.ini
        --prefix=/usr -Ddefault_library=static -Dno_headers=true
        <BINARY_DIR> <SOURCE_DIR>
        BUILD_COMMAND ${NINJA_EXE} -C <BINARY_DIR>
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${QUARK_SYSROOT} ${NINJA_EXE} -C <BINARY_DIR> install
        BUILD_BYPRODUCTS ${QUARK_SYSROOT}/usr/lib/libc.a
)

file(MAKE_DIRECTORY ${QUARK_SYSROOT}/usr/include)
add_dependencies(mlibc_headers gcc_build)
add_library(quark_libc STATIC IMPORTED GLOBAL)
set_target_properties(quark_libc PROPERTIES
        IMPORTED_LOCATION ${QUARK_SYSROOT}/usr/lib/libc.a
        INTERFACE_INCLUDE_DIRECTORIES ${QUARK_SYSROOT}/usr/include
)
add_dependencies(quark_libc mlibc_build)