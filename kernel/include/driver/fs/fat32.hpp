#pragma once
#include <cstdint>
#include <cstddef>
#include "vfs.hpp"

namespace fat32 {
    void init(const void* module_addr);
    bool lookup(uint32_t directory_cluster,
                const char* name,
                vfs::dir_entry& result);
    size_t read(const char* path, uint8_t* buffer, uint32_t max_size);
    size_t readdir(const char* path, vfs::dir_entry* entries, uint32_t max_entries);
    void write(char* path, void* buffer, uint32_t size);
}
