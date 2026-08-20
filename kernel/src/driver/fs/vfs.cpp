#include "driver/fs/vfs.hpp"

#include <boot/limine/limine.h>
#include <boot/limine/limine_requests.hpp>
#include <driver/fs/fat32.hpp>
#include <kernel/log.hpp>

class Filesystem {

};

namespace vfs {
    void init() {
        const limine_module_response* res = limine_requests::module_request.response;

        for (uint32_t i = 0; i < res->module_count; i++) {
            const limine_file* module = res->modules[i];

            logger.info("Module: %s, Address: %p, Size: %lu", module->path, module->address, module->size);
        }

        const limine_file* img_module = res->modules[0];

        fat32::init(img_module->address);
    }

    size_t stat(const char* path, vfs::stat_t* buffer) {
        return fat32::stat(path, buffer);
    }

    uint32_t open(const char* path, uint32_t flags) {
        (void)flags;
        return 0;
    }

    void close(const uint32_t handle) {
        (void)handle;
    }

    size_t read(const char* path, uint8_t* buffer, uint32_t size) {
        return fat32::read(path, buffer, size);
    }

    size_t readdir(const char* path, dir_entry* entries, uint32_t max_entries) {
        return fat32::readdir(path, entries, max_entries);
    }
}
