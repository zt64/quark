#include <cstdio>

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (remove(argv[i]) != 0) {
            fprintf(stderr, "Failed to remove %s\n", argv[i]);
        }
    }

    return 0;
}
