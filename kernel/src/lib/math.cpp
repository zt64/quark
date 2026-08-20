#include "lib/math.hpp"

int min(const int a, const int b) {
    return a < b ? a : b;
}

int max(const int a, const int b) {
    return a > b ? a : b;
}

uint64_t min(const uint64_t a, const uint64_t b) { return a < b ? a : b; }
uint64_t max(const uint64_t a, const uint64_t b) { return a > b ? a : b; }