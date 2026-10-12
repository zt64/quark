#include <boot/limine/limine.h>
#include <boot/limine/limine_requests.hpp>
#include <driver/fs/fat32.hpp>
#include <kernel/log.hpp>
#include <kernel/fs/file.hpp>
#include <kernel/fs/vfs.hpp>
#include <kernel/process/process.hpp>
#include <lib/mem.hpp>
#include <lib/printf.h>
#include <lib/string.hpp>
#include <memory/heap.hpp>

namespace vfs {
    constexpr uint16_t FD_LIMIT = 4096;

    void init() {
        const limine_module_response* res = limine_requests::module_request.response;

        // TODO: Load fs image properly
        // Parse cmdline for the path to the fs image

        const limine_file* img_module = res->modules[0];

        fat32::init(img_module->address);
    }

    /**
     * Normalizes a path by removing redundant slashes and resolving "." and ".." components.
     * @param path
     * @param resolved
     * @param size
     * @return
     */
    static char* normalize_path(const char* path) {
        if (!path) {
            logger.error("normalize_path: invalid argument");
            return nullptr;
        }

        const size_t path_len = strlen(path);

        if (path_len >= PATH_MAX) {
            logger.error("resolve_path: path too long");
            return nullptr;
        }

        const auto resolved = static_cast<char*>(kalloc(PATH_MAX));
        if (!resolved) {
            logger.error("resolve_path: allocation failed");
            return nullptr;
        }

        char c;
        size_t j = 0;

        for (size_t i = 0; i < path_len && j < PATH_MAX - 1; ++i) {
            c = path[i];

            if (c == '/') {
                // Skip redundant slashes
                while (i + 1 < path_len && path[i + 1] == '/') {
                    ++i;
                }
                resolved[j++] = '/';
            } else if (c == '.' && (i + 1 == path_len || path[i + 1] == '/')) {
                // Skip current directory references
                continue;
            } else if (c == '.' && i + 1 < path_len && path[i + 1] == '.' &&
                (i + 2 == path_len || path[i + 2] == '/')) {
                // Handle parent directory references
                if (j > 1) {
                    --j; // Remove the last '/'
                    while (j > 0 && resolved[j - 1] != '/') {
                        --j; // Remove the last component
                    }
                }
                i += 2; // Skip ".."
            } else {
                resolved[j++] = c;
            }
        }
        resolved[j] = '\0';
        return resolved;
    }

    char* resolve_path(const char* path) {
        if (!path) {
            logger.error("resolve_path: invalid argument");
            return nullptr;
        }

        const size_t path_len = strlen(path);

        if (path_len >= PATH_MAX) {
            logger.error("resolve_path: path too long");
            return nullptr;
        }

        const auto resolved = static_cast<char*>(kalloc(PATH_MAX));
        if (!resolved) {
            logger.error("resolve_path: allocation failed");
            return nullptr;
        }

        if (path[0] == '/') {
            memcpy(resolved, path, path_len + 1);
            return resolved;
        }

        const int written = snprintf(
            resolved,
            PATH_MAX,
            "%s/%s",
            current->proc->cwd,
            path
        );

        if (written < 0 || static_cast<size_t>(written) >= PATH_MAX) {
            logger.error("resolve_path: resolved path too long");
            kfree(resolved);
            return nullptr;
        }

        char* normalized = normalize_path(resolved);
        kfree(resolved);
        return normalized;
    }

    char** mounts[FD_LIMIT] = {nullptr};

    /**
     * Returns the filesystem associated with the given path. This function resolves the path and retrieves the corresponding filesystem.
     * @param path The path for which to retrieve the filesystem.
     * @return The filesystem associated with the given path, or nullptr if an error occurs.
     */
    Filesystem* get_filesystem(const char* path) {
        if (!path) {
            logger.error("get_filesystem: path is null");
            return nullptr;
        }

        char* resolved = resolve_path(path);
        if (!resolved) {
            logger.error("get_filesystem: failed to resolve path");
            return nullptr;
        }

        for (size_t i = 0; i < FD_LIMIT; ++i) {
            if (mounts[i] && strcmp(mounts[i][0], resolved) == 0) {
                kfree(resolved);
                return reinterpret_cast<Filesystem*>(mounts[i][1]);
            }
        }
        kfree(resolved);
        return nullptr;
    }

    int32_t stat(const char* path, stat_t* buffer) {
        if (!path) {
            logger.error("stat: path is null");
            return -1;
        }

        if (!buffer) {
            logger.error("stat: buffer is null");
            return -1;
        }

        char* resolved = resolve_path(path);
        if (!resolved) {
            logger.error("stat: failed to resolve path");
            return -1;
        }

        const int32_t result = fat32::stat(resolved, buffer);
        kfree(resolved);
        return result;
    }

    int32_t mkdir(const char* path, const uint32_t mode) {
        char* resolved = resolve_path(path);
        if (!resolved) {
            logger.error("mkdir: failed to resolve path");
            return -1;
        }
        const int32_t result = fat32::mkdir(resolved, mode);
        kfree(resolved);
        return result;
    }

    int32_t open(const char* path, const uint32_t flags) {
        char* resolved = resolve_path(path);
        if (!resolved) {
            logger.error("open: failed to resolve path");
            return -1;
        }
        const int32_t result = kernel::fs::open(resolved, flags, current->tid);
        kfree(resolved);
        return result;
    }

    int32_t opendir(const char* path) {
        const char* resolved = resolve_path(path);

        if (!resolved) {
            logger.error("opendir: failed to resolve path");
            return -1;
        }

        // exists
        vfs::stat_t st;
        if (vfs::stat(resolved, &st) == -1) {
            logger.error("opendir: path does not exist");
            kfree(const_cast<char*>(resolved));
            return -1;
        }

        return kernel::fs::open(resolved, kernel::fs::file_flags::READ, current->tid);
    }

    int32_t close(const size_t handle) {
        return kernel::fs::close(handle, current->tid);
    }

    int32_t closedir(const size_t handle) {
        return kernel::fs::close(handle, current->tid);
    }

    int32_t read(const char* path, uint8_t* buffer, const uint32_t size) {
        return fat32::read(resolve_path(path), buffer, size);
    }

    int32_t read(const uint32_t handle, uint8_t* buffer, const uint32_t size) {
        const kernel::fs::file* f = kernel::fs::get_file(handle, current->tid);

        if (!f) {
            return -1;
        }

        return fat32::read(f->path, buffer, size);
    }

    int32_t write(const char* path, uint8_t* buffer, const uint32_t size) {
        fat32::write(resolve_path(path), buffer, size);
    }

    int32_t write(const uint32_t handle,  uint8_t* buffer, const uint32_t size) {
        const kernel::fs::file* f = kernel::fs::get_file(handle, current->tid);

        if (!f) {
            return -1;
        }

        return write(f->path, buffer, size);
    }

    int32_t readentries(
        const uint32_t handle,
        void* buffer,
        const size_t max_size,
        size_t* bytes_read
    ) {
        kernel::fs::file* f = kernel::fs::get_file(handle, current->tid);

        if (!f) {
            *bytes_read = 0;
            return -1;
        }

        const size_t max_entries = max_size / sizeof(dirent);

        const auto entries = static_cast<dir_entry*>(
            kalloc(max_entries * sizeof(dir_entry))
        );

        // TODO: Choose the filesystem rather than hardcoding it
        const size_t entries_read = fat32::readdir(f->path, entries, max_entries, f->dir_offset);

        f->dir_offset += entries_read;

        auto* output = static_cast<dirent*>(buffer);

        for (size_t i = 0; i < entries_read; ++i) {
            output[i].d_ino = entries[i].inode;
            output[i].d_off = i + 1;
            output[i].d_reclen = sizeof(dirent);
            output[i].d_type = static_cast<unsigned char>(entries[i].type);

            strcpy(output[i].d_name, entries[i].name);
        }

        *bytes_read = entries_read * sizeof(dirent);

        kfree(entries);

        return 0;
    }
}
