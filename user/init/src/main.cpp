#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include "service.hpp"

static Service* services[256];

int main(int argc, char* argv[]) {
    printf("init start\n");

    // TODO: Iterate through services and start

    const pid_t pid = fork();

    setenv("PATH", "/bin:/usr/bin", 1);

    if (pid < 0) {
        printf("init: fork failed\n");
        for (;;) {}
    }

    if (pid == 0) {
        // Child: replace itself with snell.
        execv("/bin/snell", nullptr);
        // Only reached if execv failed
        printf("init: execv failed\n");
        exit(1);
    }

    for (;;) {
        // Init cannot die, or else the kernel dies
    }

    return 0;
}