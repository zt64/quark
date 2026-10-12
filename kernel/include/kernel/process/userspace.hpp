#pragma once
#include "process.hpp"

namespace userspace {
    /**
     * Spawn a process
     * @param path
     * @param argv
     * @param envp
     * @param env_vars
     * @return
     */
    process_t* spawn(const char* path, char** argv, char** envp, const char* const env_vars[]);
    process_t* exec(task* t, const char* path, char** argv, char** envp);
    [[noreturn]] void launch_init();
    [[noreturn]] void launch(const char* path, char** argv, char** envp, const char* const env_vars[]);
    /**
     * Enter a task, switching to userspace
     * @param task
     */
    [[noreturn]] void enter_task(task* task);
}
