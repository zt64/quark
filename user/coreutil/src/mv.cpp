#include <cstdio>

int main(const int argc, const char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: mv <source> <destination>\n");
        return 1;
    }

    const char *destination = argv[argc - 1];

    for (int i = 1; i < argc - 2; i++) {
        if (rename(argv[i], destination) != 0) {
            fprintf(stderr, "Failed to move %s to %s\n", argv[i], destination);
        }
    }

    return 0;
}
