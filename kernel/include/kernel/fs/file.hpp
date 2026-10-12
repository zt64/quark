#pragma once

#include <cstdint>

namespace kernel::fs {
    constexpr uint16_t MAX_FILES = 4096;
    constexpr uint16_t MAX_SOCKETS = 1024;

    enum file_flags : uint8_t {
        READ = 0x1,
        WRITE = 0x2,
        APPEND = 0x4,
        CREATE = 0x8,
        TRUNCATE = 0x10
    };

    struct file {
        char* path;
        uint32_t fd;
        uint32_t flags;
        uint32_t pid;
        uint32_t dir_offset;
    };

    /**
     * Open a file and return a file descriptor.
     * @param path The path to the file to open.
     * @param flags The flags to use when opening the file.
     * @param pid The process ID of the process opening the file. TODO: Remove this parameter and use the current process instead.
     * @return The file descriptor of the opened file, or a negative value on error.
     */
    int32_t open(const char* path, uint32_t flags, uint32_t pid);
    int32_t close(uint32_t fd, uint32_t pid);

    file* get_file(uint32_t fd, uint32_t pid);
}
