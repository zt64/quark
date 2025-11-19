#include <format.hpp>
#include <string.hpp>
#include <kernel.hpp>

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    const char* str = vformat(fmt, args);
    write(1, str, strlen(str));

    va_end(args);
}

int main() {
    const size_t curr_pid = getpid();

    printf("%lu: Starting", curr_pid);

    // const size_t pid = fork();

    // printf("%lu: forked new process: %lu", curr_pid, pid);

    // if (pid < 0) {
    //     printf("%lu: Fork failed", curr_pid);
    // } else if (pid == 0) {
    //     printf("%lu: I am the child process", curr_pid);
    // } else {
    //     printf("%lu: I am the parent process", curr_pid);
    // }

    for (;;) {
    }

    return 0;
}