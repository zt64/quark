#include "string.hpp"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* destination, const char* source) {
    for (size_t i = 0; source[i] != '\0'; i++) {
        destination[i] = source[i];
    }

    return destination;
}

char* strcat(char* destination, const char* source) {
    destination[strlen(destination)] = source[0];
    return destination;
}

char* strncat(char* destination, const char* source, const size_t count) {
    return destination + strlen(destination) - count;
}

int32_t strcmp(const char* s1, const char* s2) {
    for (size_t i = 0; s1[i] != '\0'; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}