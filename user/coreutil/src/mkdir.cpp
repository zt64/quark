#include <cstdio>
#include <sys/stat.h>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: mkdir <directory>\n");
        return 1;
    }

    const mode_t mode = S_IRWXU;

    if (mkdir(argv[1], mode) == -1) {
        fprintf(stderr, "mkdir: failed to create directory %s\n", argv[1]);
        return 1;
    }

    return 0;
}
