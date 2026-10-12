#include "kernel/process/userspace.hpp"

#include <kernel/fs/vfs.hpp>
#include <kernel/log.hpp>
#include <kernel/process/process.hpp>
#include <kernel/process/scheduler.hpp>
#include <memory/heap.hpp>
#include <lib/string.hpp>

namespace userspace {
    namespace {
        // Reads a whole file into a freshly kalloc'd buffer for the ELF
        // loader to consume. Returns 0 on failure.
        uintptr_t load_program_buffer(const char* path, const char* caller) {
            vfs::stat_t stat = {};
            if (vfs::stat(path, &stat) < 0) {
                logger.error("%s: failed to stat %s", caller, path);
                return 0;
            }

            const uint32_t max_buffer_size = stat.size;
            const auto program_buf = static_cast<uint8_t*>(kalloc(max_buffer_size));
            if (!program_buf) {
                logger.error("%s: failed to allocate buffer for %s", caller, path);
                return 0;
            }
            if (!vfs::read(path, program_buf, max_buffer_size)) {
                logger.error("%s: failed to read %s", caller, path);
                kfree(program_buf);
                return 0;
            }

            return reinterpret_cast<uintptr_t>(program_buf);
        }
    }

    char** copy_user_strv(char** src) {
        if (!src) {
            auto* copy = static_cast<char**>(kalloc(sizeof(char*)));
            copy[0] = nullptr;
            return copy;
        }

        size_t count = 0;
        while (src[count] != nullptr) count++;

        auto* copy = static_cast<char**>(kalloc((count + 1) * sizeof(char*)));
        if (!copy) return nullptr;

        for (size_t i = 0; i < count; i++) {
            copy[i] = strdup(src[i]);
        }
        copy[count] = nullptr;

        return copy;
    }

    process_t* spawn(const char* path, char** argv, char** envp, const char* const env_vars[]) {
        const uintptr_t program_addr = load_program_buffer(path, "spawn");
        if (program_addr == 0) {
            return nullptr;
        }

        // argv[0] must always be the program name; synthesize it when the
        // caller doesn't provide an argv.
        char* default_argv[] = {const_cast<char*>(path), nullptr};
        if (!argv) argv = default_argv;

        process_t* p = process::create_task(program_addr, argv, envp);
        p->executable = strdup(path);
        p->argv = copy_user_strv(argv);
        p->envp = copy_user_strv(envp);

        scheduler::schedule_task(p->main_task);

        return p;
    }

    process_t* exec(task* t, const char* path, char** argv, char** envp) {
        const uintptr_t program_addr = load_program_buffer(path, "exec");

        if (program_addr == 0) return nullptr;

        // argv[0] must always be the program name; synthesize it when the
        // caller doesn't provide an argv.
        char* default_argv[] = {const_cast<char*>(path), nullptr};
        if (!argv) argv = default_argv;

        return process::exec_current(t, program_addr, path, argv, envp);
    }
}
