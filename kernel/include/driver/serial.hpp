#pragma once

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

    void putchar(char c);

    void print(const char* str);

    __attribute__ ((format (printf, 1, 2)))
    void printf(const char* fmt, ...);

    // Returns true if serial was successfully initialized and is usable
    bool available();
}