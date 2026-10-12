cmake_minimum_required(VERSION 3.20)
project(libc LANGUAGES C CXX)

add_library(quark_libc STATIC IMPORTED GLOBAL)
set_target_properties(quark_libc PROPERTIES
        IMPORTED_LOCATION ${QUARK_SYSROOT}/usr/lib/libc.a
)
add_dependencies(quark_libc)