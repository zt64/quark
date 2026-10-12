#include <cstdio>
#include <sys/utsname.h>

int main() {
    utsname uts;

    const int ret = uname(&uts);
    if (ret == 0) {
        printf("System name: %s\n", uts.sysname);
    } else {
        fprintf(stderr, "uname failed\n");
        return 1;
    }
    return 0;
}
