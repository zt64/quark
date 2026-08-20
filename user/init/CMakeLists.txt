add_executable(init src/main.cpp)
target_link_libraries(init PRIVATE quark_userspace_options quark_libc)
install(TARGETS init RUNTIME DESTINATION boot)