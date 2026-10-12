#include <kernel/fs/file.hpp>
#include <lib/string.hpp>
#include <memory/heap.hpp>

namespace kernel::fs {
    file files[MAX_FILES];

    // TODO: Make this per-process.
    uint32_t current_fd = 3;

    int32_t open(const char* path, const uint32_t flags, uint32_t pid) {
        if (current_fd >= MAX_FILES)
            return -1;

        const uint32_t fd = current_fd++;

        file f{};
        f.fd = fd;
        f.flags = flags;
        f.pid = pid;
        f.path = strdup(path);
        f.dir_offset = 0;

        files[fd] = f;

        return fd;
    }

    int32_t close(const uint32_t fd, const uint32_t pid) {
        if (fd >= MAX_FILES)
            return -1;

        file& f = files[fd];

        if (f.fd != fd || f.pid != pid)
            return -1;

        kfree(f.path);
        f = {};

        return 0;
    }
    file* get_file(const uint32_t fd, const uint32_t pid) {
        if (fd >= MAX_FILES)
            return nullptr;

        file& f = files[fd];

        if (f.fd != fd || f.pid != pid)
            return nullptr;

        return &f;
    }

}
