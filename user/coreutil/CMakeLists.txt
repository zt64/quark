set(COREUTILS
        cat
        env
        id
        kill
        mv
        mkdir
        cp
        rm
        touch
        whoami
        date
        fetch
        free
        sleep
        uname
)

foreach(tool IN LISTS COREUTILS)
    add_executable(${tool} src/${tool}.cpp)

    target_link_libraries(${tool}
            PRIVATE
            quark_userspace_options
            quark_libc
    )

    install(TARGETS ${tool} RUNTIME DESTINATION bin)
endforeach()

add_custom_target(coreutils)
add_dependencies(coreutils ${COREUTILS})