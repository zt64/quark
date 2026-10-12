#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Count
 * @param str
 * @return
 */
size_t strlen(const char* str);

char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, size_t count);

char* strdup(const char* src);

char* strcat(char* destination, const char* source);
char* strncat(char* destination, const char* source, size_t count);

int32_t strcmp(const char* s1, const char* s2);
int32_t strncmp(const char* s1, const char* s2, size_t n);

int32_t strcasecmp(const char* a, const char* b);

char* strtok(char* str, const char* delimiters);

char* strtok_r(char* str, const char* delim, char** saveptr);
