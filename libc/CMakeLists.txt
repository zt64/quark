cmake_minimum_required(VERSION 3.20)
project(libc LANGUAGES C CXX)

file(MAKE_DIRECTORY ${QUARK_SYSROOT}/usr/include)

add_library(quark_libc STATIC IMPORTED GLOBAL)
set_target_properties(quark_libc PROPERTIES
        IMPORTED_LOCATION ${QUARK_SYSROOT}/usr/lib/libc.a
        INTERFACE_INCLUDE_DIRECTORIES "${QUARK_SYSROOT}/usr/include;${CMAKE_CURRENT_SOURCE_DIR}/include"
)
add_dependencies(quark_libc)