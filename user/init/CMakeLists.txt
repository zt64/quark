add_executable(init src/main.cpp src/service.cpp)
target_include_directories(init PRIVATE include)
target_link_libraries(init PRIVATE quark_userspace_options quark_libc)
install(TARGETS init RUNTIME DESTINATION bin)