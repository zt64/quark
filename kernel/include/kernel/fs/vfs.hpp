#pragma once
#include <cstddef>
#include <cstdint>
#include <kernel/fs/file.hpp>

namespace vfs {
    constexpr uint16_t PATH_MAX = 4096;

    struct dir {
        char name[255];
        uint32_t file_count;
        kernel::fs::file* files;
    };

    struct timespec {
        int64_t tv_sec; /* seconds */
        int64_t tv_nsec; /* nanoseconds */
    };

    struct stat_t {
        uint64_t st_dev; /* ID of device containing file */
        uint64_t inode; /* Inode number */
        uint64_t st_nlink; /* Number of hard links */
        uint32_t st_mode; /* File type and mode */
        uint32_t st_uid; /* User ID of owner */
        uint32_t st_gid; /* Group ID of owner */
        uint32_t _pad;
        uint64_t st_rdev; /* Device ID (if special file) */
        int64_t size; /* Total size, in bytes */
        int64_t st_blksize; /* Block size for filesystem I/O */
        int64_t st_blocks; /* Number of 512 B blocks allocated */
        timespec st_atim; /* Time of last access */
        timespec st_mtim; /* Time of last modification */
        timespec st_ctim; /* Time of last status change */
        int64_t unused[3];
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

    struct dirent {
        uint64_t d_ino;
        uint64_t d_off;
        uint16_t d_reclen;
        unsigned char d_type;
        char d_name[255 + 1];
    };

    void init();

    void mount(const char* path);
    void unmount();

    char* resolve_path(const char* path);

    size_t stat(size_t handle, stat_t* buffer);
    int32_t stat(const char* path, stat_t* buffer);
    int32_t open(const char* path, uint32_t flags);
    int32_t opendir(const char* path);
    int32_t read(const char* path, uint8_t* buffer, uint32_t size);
    int32_t read(uint32_t handle, uint8_t* buffer, uint32_t size);
    int32_t write(const char* path,  uint8_t* buffer, uint32_t size);
    int32_t write(uint32_t handle,  uint8_t* buffer, uint32_t size);
    int32_t readentries(uint32_t handle,
                       void* buffer, size_t max_size,
                       size_t* bytes_read);
    int32_t mkdir(const char* path, uint32_t mode);
    int32_t close(size_t handle);
    int32_t closedir(size_t handle);
}
