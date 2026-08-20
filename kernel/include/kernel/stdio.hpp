#pragma once

#include <cstddef>
#include <cstdint>

size_t write_stdin(const uint8_t* data, size_t count);
size_t write_stdout(const uint8_t* data, size_t count);

void init_stdio();
void stdin_push(uint8_t c);
void stdout_push(const uint8_t *c);
void stderr_push(uint8_t c);

size_t stdin_read(void* buf, size_t size);
int stdout_read(void* buf, size_t size);
int stderr_read(void* buf, size_t size);