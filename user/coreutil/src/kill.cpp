#include <cstdio>
#include <signal.h>
#include <abi-bits/pid_t.h>

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: kill <pid>\n");
        return 1;
    }

    pid_t pid;

    if (sscanf(argv[1], "%d", &pid) != 1) {
        fprintf(stderr, "kill: invalid pid: %s\n", argv[1]);
        return 1;
    }

    int num = 9;

    if (argc == 3) {

        if (sscanf(argv[2], "%d", &num) != 1) {
            fprintf(stderr, "kill: invalid signal number: %s\n", argv[2]);
            return 1;
        }

    }

    if (kill(pid, num) == 0) {
        printf("Sent signal %d to process %d\n", num, pid);
    } else {
        perror("kill");
        return 1;
    }

    return 0;
}
