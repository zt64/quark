#include "stdio.h"
#include "unistd.h"

int main() {
    const size_t curr_pid = getpid();

    write(1, "hi", 2);

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
