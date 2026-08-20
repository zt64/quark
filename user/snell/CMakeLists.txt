add_executable(snell src/main.cpp)
target_link_libraries(snell PRIVATE quark_userspace_options quark_libc)
install(TARGETS snell RUNTIME DESTINATION boot)