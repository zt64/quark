#include "rand.hpp"
#include <cstdint>

static uint64_t next = 1;

int rand() {
    next = next * 1103515245 + 12345;
    return static_cast<int>(next / UINT16_MAX) % RAND_MAX;
}

void srand(const unsigned int seed) {
    next = seed;
}
