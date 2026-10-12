#include <cstdio>
#include <cstdlib>

extern char **environ;

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: env <key>=<value> [<key>=<value> ...]\n");
        return 1;
    }

    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }

    return EXIT_SUCCESS;
}
