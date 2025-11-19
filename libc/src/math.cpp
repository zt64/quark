#include "math.hpp"

// float floor(const float num) {
//     const int i = static_cast<int>(num);
//     return (num < static_cast<float>(i)) ? static_cast<float>(i - 1) : static_cast<float>(i);
// }

// float ceil(const float num) {
//
// }

int min(const int a, const int b) {
    return a < b ? a : b;
}

int max(const int a, const int b) {
    return a > b ? a : b;
}