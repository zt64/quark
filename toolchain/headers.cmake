find_program(MESON_EXE NAMES meson REQUIRED)
find_program(NINJA_EXE NAMES ninja REQUIRED)

configure_file(
        ${CMAKE_SOURCE_DIR}/toolchain/quark-cross.ini.in
        ${CMAKE_BINARY_DIR}/quark-cross.ini
        @ONLY
)

ExternalProject_Add(mlibc_headers
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/libc/mlibc
        CONFIGURE_COMMAND ${MESON_EXE} setup
        --cross-file ${CMAKE_BINARY_DIR}/quark-cross.ini
        --prefix=/usr -Dheaders_only=true
        <BINARY_DIR> <SOURCE_DIR>
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${QUARK_SYSROOT} ${NINJA_EXE} -C <BINARY_DIR> install
)