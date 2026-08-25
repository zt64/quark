#pragma once
#include <cstddef>
#include <cstdint>

namespace vfs {
    struct file {
        char name[255];
        uint32_t size;
    };

    struct dir {
        char name[255];
        uint32_t file_count;
        file* files;
    };

    struct stat_t {
        uint64_t inode;
        uint64_t size;
    };

    enum class file_type : uint8_t {
        Regular,
        Directory,
        Symlink,
        BlockDevice,
        CharacterDevice,
        Fifo,
        Socket,
        Unknown
    };

    struct dir_entry {
        char name[255];
        file_type type;
        uint64_t size;
        uint64_t inode;
    };

    void init();

    void mount(const char* path);

    size_t stat(const char* path, stat_t* buffer);
    uint32_t open(const char* path, uint32_t flags);
    size_t opendir(const char* path);
    size_t read(const char* path, uint8_t* buffer, uint32_t size);
    size_t readdir(const char* path, dir_entry* entries, uint32_t max_entries);
    size_t close(size_t handle);
    size_t closedir(size_t handle);
}
