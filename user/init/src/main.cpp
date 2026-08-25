#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    execv("/BOOT/SNELL", nullptr);

    // Init cannot die, or else the kernel dies
    for (;;) {

    }

    return 0;
}