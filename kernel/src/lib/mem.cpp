#include "lib/mem.hpp"
#include <cstddef>
#include <cstdint>

void* memcpy(void* dst_ptr, const void* src_ptr, size_t n) {
    auto* dst = static_cast<uint8_t*>(dst_ptr);
    const auto* src = static_cast<const uint8_t*>(src_ptr);

    while (n--)
        *dst++ = *src++;

    return dst_ptr;
}

void* memmove(void* dst_ptr, const void* src_ptr, size_t n) {
    auto* dst = static_cast<uint8_t*>(dst_ptr);
    const auto* src = static_cast<const uint8_t*>(src_ptr);

    if (dst == src || n == 0)
        return dst_ptr;

    if (dst < src) {
        while (n--)
            *dst++ = *src++;
    } else {
        dst += n;
        src += n;

        while (n--) {
            *--dst = *--src;
        }
    }

    return dst_ptr;
}

int32_t memcmp(const void* a, const void* b, size_t n) {
    const auto* p1 = static_cast<const uint8_t*>(a);
    const auto* p2 = static_cast<const uint8_t*>(b);

    while (n--) {
        if (*p1 != *p2)
            return static_cast<int32_t>(*p1) - static_cast<int32_t>(*p2);

        ++p1;
        ++p2;
    }

    return 0;
}

void* memset(void* dst_ptr, uint8_t value, size_t n) {
    auto* dst = static_cast<uint8_t*>(dst_ptr);

    while (n--)
        *dst++ = value;

    return dst_ptr;
}
