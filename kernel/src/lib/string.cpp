#include "lib/string.hpp"
#include "lib/mem.hpp"
#include "memory/heap.hpp"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* destination, const char* source) {
    size_t i = 0;

    while ((destination[i] = source[i]) != '\0') {
        i++;
    }

    return destination;
}

char* strncpy(char* destination, const char* source, const size_t count) {
    size_t i = 0;

    while (i < count && (destination[i] = source[i]) != '\0') {
        i++;
    }

    // pads the destination with null bytes if source is shorter than count
    while (i < count) {
        destination[i++] = '\0';
    }

    return destination;
}

char* strdup(const char* src) {
    if (src == nullptr) {
        return nullptr;
    }

    const size_t len = strlen(src) + 1; // include null terminator
    auto* copy = static_cast<char*>(kalloc(len));
    if (copy == nullptr) {
        return nullptr;
    }

    memcpy(copy, src, len);
    return copy;
}

char* strcat(char* destination, const char* source) {
    char* end = destination + strlen(destination);

    while ((*end++ = *source++) != '\0') {
    }

    return destination;
}

char* strncat(char* destination, const char* source, size_t count) {
    char* end = destination + strlen(destination);

    while (count-- && *source) {
        *end++ = *source++;
    }

    *end = '\0';

    return destination;
}

int32_t strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return static_cast<unsigned char>(*s1) - static_cast<unsigned char>(*s2);
}

int32_t strncmp(const char* s1, const char* s2, size_t n) {
    while (n-- && *s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return static_cast<unsigned char>(*s1) - static_cast<unsigned char>(*s2);
}

int32_t strcasecmp(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a;
        char cb = *b;

        if (ca >= 'a' && ca <= 'z') ca -= 'a' - 'A';
        if (cb >= 'a' && cb <= 'z') cb -= 'a' - 'A';

        if (ca != cb) {
            return static_cast<unsigned char>(ca) - static_cast<unsigned char>(cb);
        }

        ++a;
        ++b;
    }

    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}
