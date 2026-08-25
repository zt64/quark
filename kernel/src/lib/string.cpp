#include "lib/string.hpp"
#include "lib/mem.hpp"
#include "lib/stdlib.hpp"

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

char* strdup(const char* src) {
    if (src == nullptr) {
        return nullptr;
    }

    const size_t len = strlen(src) + 1; // include null terminator
    auto* copy = static_cast<char*>(malloc(len));
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

    return static_cast<unsigned char>(*s1) -
        static_cast<unsigned char>(*s2);
}
