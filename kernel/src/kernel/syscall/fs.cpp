#include "kernel/syscall/internal.hpp"
#include <lib/mem.hpp>
#include <lib/string.hpp>
#include <kernel/log.hpp>
#include <kernel/fs/vfs.hpp>
#include <kernel/process/process.hpp>
#include <kernel/syscall/syscall.hpp>
#include <memory/paging.hpp>
#include <lib/printf.h>

static int32_t sys_opendir(const char* path, int* handle) {
    if (!path) {
        logger.error("opendir: path is null");
        return -1;
    }

    if (!handle) {
        logger.error("opendir: handle is null");
        return -1;
    }

    const int result = vfs::opendir(path);

    if (result < 0) return -1;

    *handle = result;
    return 0;
}

static int32_t sys_readdir(const int handle, void* dirent_buf) {
    if (handle < 0) {
        logger.error("readdir: invalid handle %d", handle);
        return -1;
    }

    // vfs::readdir(handle, dirent_buf);

    return -1;
}

static int32_t sys_closedir(const int handle) {
    if (handle < 0) {
        logger.error("closedir: invalid handle %d", handle);
        return -1;
    }

    return vfs::closedir(handle);
}

static int32_t sys_read_entries(const int handle,
                                void* buffer,
                                const size_t max_size,
                                size_t* bytes_read) {
    return vfs::readentries(handle, buffer, max_size, bytes_read);
}

static int32_t sys_stat(const char* path, vfs::stat_t* buffer) {
    if (!path) {
        logger.error("stat: path is null");
        return -1;
    }

    if (!buffer) {
        logger.error("stat: buffer is null");
        return -1;
    }

    return vfs::stat(path, buffer);
}

static int32_t sys_chdir(const char* path) {
    if (!path) {
        logger.error("chdir: path is null");
        return -1;
    }

    const char* resolved = vfs::resolve_path(path);

    if (!resolved) {
        return -1;
    }

    vfs::stat_t st;

    if (vfs::stat(resolved, &st) == -1) {
        return -1;
    }

    // if (!st.is_directory) {
    //     logger.error("chdir: path is not a directory");
    //     return -1;
    // }

    memcpy(
        current->proc->cwd,
        resolved,
        strlen(resolved) + 1
    );

    return 0;
}

typedef uint32_t mode_t;

static int64_t sys_mkdir(const char* path, const mode_t mode) {
    if (!path) {
        logger.error("mkdir: path is null");
        return -1;
    }

    return vfs::mkdir(path, mode);
}

static int32_t sys_getcwd(char* buffer) {
    if (!buffer) {
        logger.error("getcwd: buffer is null");
        return -1;
    }

    const size_t len = strlen(current->proc->cwd);

    memcpy(buffer, current->proc->cwd, len + 1);

    return 0;
}

SYSCALL_HANDLER(opendir);
SYSCALL_HANDLER(readdir);
SYSCALL_HANDLER(closedir);
// SYSCALL_HANDLER(readentries);
SYSCALL_HANDLER(stat);
SYSCALL_HANDLER(chdir);
SYSCALL_HANDLER(mkdir);
SYSCALL_HANDLER(getcwd);

int64_t handle_readentries(const regs* r) {
    // buffer and bytes_read are userspace pointers; translate them before the
    // kernel writes through them (same class of bug as handle_getstat).
    uintptr_t buffer_phys;
    if (!paging::translate(current->proc->cr3, r->rsi, buffer_phys)) {
        logger.error("readentries: failed to translate buffer pointer");
        return -1;
    }

    uintptr_t bytes_read_phys;
    if (!paging::translate(current->proc->cr3, r->r10, bytes_read_phys)) {
        logger.error("readentries: failed to translate bytes_read pointer");
        return -1;
    }

    return sys_read_entries(
        static_cast<int>(r->rdi),
        reinterpret_cast<void*>(buffer_phys + paging::g_hhdm_offset),
        r->rdx,
        reinterpret_cast<size_t*>(bytes_read_phys + paging::g_hhdm_offset)
    );
}

int64_t handle_getstat(const regs* r) {
    // r->rsi is a userspace pointer; it must be translated to a physical
    // (HHDM-mapped) address before the kernel writes through it, otherwise
    // the write lands on whatever happens to be mapped at that virtual
    // address in the kernel's current address space, corrupting unrelated
    // memory instead of the caller's actual stat buffer.
    uintptr_t buffer_phys;
    if (!paging::translate(current->proc->cr3, r->rsi, buffer_phys)) {
        logger.error("stat: failed to translate buffer pointer");
        return -1;
    }

    return sys_stat(
        reinterpret_cast<const char*>(r->rdi),
        reinterpret_cast<vfs::stat_t*>(buffer_phys + paging::g_hhdm_offset)
    );
}