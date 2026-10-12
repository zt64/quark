#pragma once

#include <cstdint>
#include <kernel/fs/vfs.hpp>

class Filesystem {
public:
    virtual ~Filesystem() = default;

    virtual void init() = 0;

    virtual int32_t stat(const char* path, vfs::stat_t* result) = 0;
    virtual int32_t read(const char* path, uint8_t* buffer, uint32_t max_size) = 0;
    virtual int32_t write(const char* path, const void* buffer, uint32_t size) = 0;
    virtual int32_t readdir(const char* path, vfs::dir_entry* entries,
                            uint32_t max_entries, uint32_t offset) = 0;
    virtual int32_t mkdir(const char* path, uint32_t mode) = 0;
    virtual int32_t rmdir(const char* path) = 0;
    virtual int32_t unlink(const char* path) = 0;
    virtual bool lookup(const char* path, vfs::dir_entry& result) = 0;
};
