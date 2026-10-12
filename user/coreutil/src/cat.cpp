#include <cstdio>
#include <sys/stat.h>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cat <file>\n");
        return 1;
    }

    const char* path = argv[1];

    struct stat buf;
    if (stat(path, &buf) == -1) {
        fprintf(stderr, "cat: failed to stat %s\n", path);
        return 1;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "cat: failed to open %s\n", path);
        return 1;
    }

    constexpr size_t buffer_size = 4096;
    char buffer[buffer_size];

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, buffer_size, f)) > 0) {
        fwrite(buffer, 1, bytes_read, stdout);
    }

    fclose(f);

    return 0;
}
