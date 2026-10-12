#include <driver/serial.hpp>
#include <kernel/console.hpp>
#include <kernel/log.hpp>
#include <kernel/fs/vfs.hpp>
#include <kernel/syscall/syscall.hpp>

#include "kernel/stdio.hpp"
#include "kernel/syscall/internal.hpp"

static size_t sys_read(const uint32_t fd, void* buf, const size_t count) {
    if (buf == nullptr) {
        return 0;
    }
    switch (fd) {
        case 0:
            return stdin_read(buf, count);
        case 1:
            panic("Not implemented");
        default:
            vfs::read(fd, static_cast<uint8_t*>(buf), count);
            return 0;
    }
}

static size_t sys_write(const uint32_t fd,  void* buf, const size_t count) {
    switch (fd) {
        case 0:
            if (buf && count > 0) {
                return write_stdin(static_cast<const uint8_t*>(buf), count);
            }
            break;
        case 1:
            if (buf && count > 0) {
                console::write(static_cast<const char*>(buf), count);
                return count;
            }
            break;
        case 2:
            if (buf && count > 0) {
                console::write(static_cast<const char*>(buf), count);
                serial::print(static_cast<const char*>(buf), count);
                return count;
            }
            break;
        default:
            vfs::write(fd, static_cast<uint8_t*>(buf), count);
            break;
    }
    return 0;
}

static size_t sys_open(const char* pathname, const int flags, void* mode, int* fd) {
    *fd = vfs::open(pathname, flags);

    if (*fd < 0) {
        logger.error("sys_open: failed to open file %s", pathname);
        return static_cast<size_t>(-1);
    }

    return 0;
}

static size_t sys_close(const uint32_t fd) {
    return vfs::close(fd);
}

SYSCALL_HANDLER(read);
SYSCALL_HANDLER(write);
SYSCALL_HANDLER(open);
SYSCALL_HANDLER(close);
