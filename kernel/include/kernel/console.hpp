#pragma once

#include <cstddef>

namespace console {
    inline bool initialized = false;

    void init();
    void write(const char* str, size_t len);

    /**
     * Clear the console screen and reset the cursor position to the top-left corner
     */
    void clear();
}
