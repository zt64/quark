#include <cstdio>
#include <cstdlib>
#include <unistd.h>

int main(const int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: sleep <seconds>\n");
        return EXIT_FAILURE;
    }

    const int seconds = atoi(argv[1]);

    if (seconds < 0) {
        fprintf(stderr, "sleep: invalid time interval '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    sleep(seconds);

    return EXIT_SUCCESS;
}
