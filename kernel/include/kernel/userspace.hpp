#pragma once
#include "process.hpp"

namespace userspace {
    [[noreturn]] void launch_init();
    void launch(const char* path, const char* const argv[], char* const envp[], const char* const env_vars[]);
    [[noreturn]] void enter_task(task* task);
}
