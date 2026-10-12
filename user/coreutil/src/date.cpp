#include <cstdio>

#include "time.h"

int main() {
    timespec spec;

    if (const int err = timespec_get(&spec, TIME_UTC); err == 0) {
        fprintf(stderr, "timespec_get failed: %d\n", err);
    };

    printf("%ld %ld\n", spec.tv_sec, spec.tv_nsec);

    return 0;
}
