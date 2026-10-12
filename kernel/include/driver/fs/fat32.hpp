#pragma once

#include <cstddef>
#include <driver/fs/fs.hpp>

#include "kernel/fs/vfs.hpp"

namespace fat32 {
    class Fat32Filesystem : public Filesystem {
    public:
        void init() override;
    };

    void init(const void* module_addr);
    bool lookup(uint32_t directory_cluster,
                const char* name,
                vfs::dir_entry& result);
    size_t stat(const char* path, vfs::stat_t* result);
    size_t read(const char* path, uint8_t* buffer, uint32_t max_size);
    size_t readdir(
        const char* path,
        vfs::dir_entry* entries,
        uint32_t max_entries,
        uint32_t offset
    );
    void write(char* path, void* buffer, uint32_t size);
    int32_t mkdir(const char* path, uint32_t mode);
}
