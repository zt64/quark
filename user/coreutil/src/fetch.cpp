#include <cstdio>

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: fetch <url>\n");
        return 1;
    }

    printf("fetch %s\n", argv[1]);
}
