#include <cstdio>

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: touch <file>\n");
        return 1;
    }

    const char *filename = argv[1];

    FILE *file = fopen(filename, "a");

    if (!file) {
        perror("fopen");
        return 1;
    }

    fclose(file);

    return 0;
}
