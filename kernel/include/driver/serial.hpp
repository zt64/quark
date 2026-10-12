#pragma once

#include <cstddef>

namespace serial {
    /**
     *
     * @return 0 if successful, 1 if faulty
     */
    int init();

    bool received();

    /**
     * Read a single character
     * @return
     */
    char read();

    bool transmitted();

    /**
     * Print a single character to the serial port
     * @param c The character to print
     */
    void putchar(char c);

    /**
     * Print a string to the serial port
     * @param str
     */
    void print(const char* str);

    /**
     * Print a fixed number of bytes to the serial port. Unlike print(), this
     * does not require (or stop at) a null terminator, so it is safe to use
     * with raw, non-null-terminated buffers such as syscall write() buffers.
     * @param str
     * @param len
     */
    void print(const char* str, size_t len);

    __attribute__ ((format (printf, 1, 2)))
    void printf(const char* fmt, ...);

    // Returns true if serial was successfully initialized and is usable
    bool available();
}
