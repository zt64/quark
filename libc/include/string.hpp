#pragma once

#include <cstddef>
#include <sys/types.h>

size_t strlen(const char* str);

char* strcpy(char* destination, const char* source);

char* strcat(char* destination, const char* source);

char* strncat(char* destination, const char* source, size_t count);

int32_t strcmp(const char* s1, const char* s2);

char* strtok(char* str, const char* delimiters);

char *strtok_r(char *str, const char *delim, char **saveptr);