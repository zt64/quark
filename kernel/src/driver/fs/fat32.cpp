#include "driver/fs/fat32.hpp"
#include <cstddef>
#include <driver/fs/vfs.hpp>
#include <kernel/log.hpp>
#include <lib/mem.hpp>
#include <lib/string.hpp>

// bios parameter block
struct bpb {
    uint8_t unk[3];
    char oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fats;
    uint16_t root_entries;
    uint16_t sectors;
    uint8_t media_descriptor_type;
    uint16_t table_size_16; // FAT12/FAT16 only!!
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;
} __attribute__((packed));

// extended boot record
struct ebr_32 {
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t nt_flags; // Windows NT flags, not used in Quark
    uint8_t signature;
    uint32_t volume_serial_number;
    char volume_label[11];
    char system_identifier[8];
    uint8_t boot_code[420];
    uint16_t boot_signature;
} __attribute__((packed));

struct fs_info_t {
    uint32_t lead_signature;
    uint8_t reserved[480];
    uint32_t signature;
    uint32_t free_clusters;
    uint32_t cluster_number_start;
    uint8_t reserved2[12];
    uint32_t trail_signature;
} __attribute__((packed));

struct date {
};

struct time {
};

struct long_file_name {
    uint8_t order;
    char16_t name_1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    char16_t name_2[6];
    uint16_t first_cluster;
    char16_t name_3[2];
} __attribute__((packed));;

enum attribute : uint8_t {
    READ_ONLY = 0x01,
    HIDDEN = 0x02,
    SYSTEM = 0x04,
    VOLUME_ID = 0x08,
    DIRECTORY = 0x10,
    ARCHIVE = 0x20
};

struct dir_entry {
    char name[11];
    attribute attr;
    uint8_t reserved;
    uint8_t creation_time_hundreds;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_num_high;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_num_low;
    uint32_t size;
} __attribute__((packed));

enum fat_type {
    ExFAT,
    FAT12,
    FAT16,
    FAT32
};

namespace fat32 {
    constexpr uint32_t EOF_MARKER = 0x0FFFFFF8;

    struct fat32_volume {
        const void* module_addr;
        uint32_t first_fat_sector;
        uint16_t bytes_per_sector;
        uint32_t entries_per_cluster;
        uint32_t sectors_per_cluster;
        uint32_t first_data_sector;
        uint32_t root_cluster;
    };

    fat32_volume volume;

    inline uint32_t calculate_lba(const fat32_volume& volume, const uint32_t cluster) {
        return ((cluster - 2) * volume.sectors_per_cluster) +
            volume.first_data_sector;
    }

    constexpr uint8_t LAST_LFN_ENTRY = 0x40;

    static uint32_t read_fat_entry(const void* module_addr, const uint32_t first_fat_sector,
                                   const uint16_t bytes_per_sector, const uint32_t cluster) {
        // Each FAT32 entry is 4 bytes. Find which sector of the FAT holds this
        // cluster's entry, and the byte offset within that sector.
        const uint32_t fat_offset = cluster * 4;
        const uint32_t fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
        const uint32_t entry_offset = fat_offset % bytes_per_sector;

        const uint8_t* fat_entry_addr = static_cast<const uint8_t*>(module_addr)
            + static_cast<size_t>(fat_sector) * bytes_per_sector
            + entry_offset;

        uint32_t raw;
        memcpy(&raw, fat_entry_addr, sizeof(raw));

        return raw & 0x0FFFFFFF; // mask off reserved top 4 bits
    }

    size_t read_directory(uint32_t cluster, vfs::dir_entry* entries, const uint32_t max_entries) {
        size_t entry_count = 0;

        while (cluster < EOF_MARKER) {
            const uint32_t lba = calculate_lba(volume, cluster);
            for (uint32_t j = 0; j < volume.entries_per_cluster; j++) {
                const uint8_t* start = static_cast<const uint8_t*>(volume.module_addr)
                    + lba * volume.bytes_per_sector
                    + sizeof(dir_entry) * j;

                if (start[0] == 0) goto done; // 0x00 = no more entries, anywhere
                if (static_cast<uint8_t>(start[0]) == 0xE5) continue; // deleted entry

                const auto entry = reinterpret_cast<const dir_entry*>(start);
                if (entry->attr == 0x0F) continue; // LFN fragment, not a real entry

                char name[255] = {};
                int name_len = 0;
                if (j > 0) {
                    uint32_t k = j;
                    while (k > 0) {
                        const uint8_t* lfn_ptr = start - sizeof(dir_entry) * (j - k + 1);
                        const auto* lfn = reinterpret_cast<const long_file_name*>(lfn_ptr);
                        if (lfn->attr != 0x0F) break;

                        char16_t chunk[13];
                        memcpy(chunk, lfn->name_1, 5 * sizeof(char16_t));
                        memcpy(&chunk[5], lfn->name_2, 6 * sizeof(char16_t));
                        memcpy(&chunk[11], lfn->name_3, 2 * sizeof(char16_t));

                        for (int c = 0; c < 13; c++) {
                            if (chunk[c] == 0x0000 || chunk[c] == 0xFFFF) break;
                            name[name_len++] = static_cast<char>(chunk[c] & 0xFF);
                        }

                        k--;
                        if (lfn->order & LAST_LFN_ENTRY) break;
                    }
                }

                if (name_len > 0) {
                    name[name_len] = '\0';
                } else {
                    memcpy(name, entry->name, 11);
                    uint16_t len = 11;
                    while (len > 0 && name[len - 1] == ' ') len--;
                    name[len] = '\0';
                }

                const uint32_t entry_cluster = (entry->cluster_num_high << 16) | entry->cluster_num_low;

                if (entry_count >= max_entries)
                    return entry_count;

                vfs::dir_entry* vfs_entry = &entries[entry_count++];

                strcpy(vfs_entry->name, name);
                vfs_entry->size = entry->size;
                vfs_entry->inode = entry_cluster;
                vfs_entry->type = entry->attr & DIRECTORY ? vfs::file_type::Directory : vfs::file_type::Regular;
            }

            cluster = read_fat_entry(volume.module_addr, volume.first_fat_sector, volume.bytes_per_sector, cluster);
        }
    done:;
        return entry_count;
    }

    bool next_component(const char** path, char* component) {
        const char* p = *path;

        // Skip leading slashes
        while (*p == '/') p++;

        if (*p == '\0') return false; // No more components

        const char* start = p;
        while (*p != '/' && *p != '\0') p++;

        size_t len = p - start;
        if (len >= 256) len = 255; // Prevent buffer overflow
        memcpy(component, start, len);
        component[len] = '\0';

        *path = p; // Update the path pointer to the next component
        return true;
    }

    bool resolve_path(const char* path, vfs::dir_entry& result) {
        uint32_t cluster = volume.root_cluster;

        logger.debug("Resolving path: %s", path);

        if (strcmp(path, "/") != 0) {
            // walk each path component
            char component[256];
            const char* p = path;

            while (next_component(&p, component)) {
                if (!lookup(cluster, component, result)) {
                    return false;
                }

                cluster = static_cast<uint32_t>(result.inode);
            }
        } else {
            // Root directory
            result.name[0] = '/';
            result.name[1] = '\0';
            result.type = vfs::file_type::Directory;
            result.size = 0;
            result.inode = volume.root_cluster;
        }

        logger.debug("Resolved path '%s' to cluster %u", path, cluster);

        return true;
    }

    size_t stat(const char* path, vfs::stat_t* result) {
        vfs::dir_entry entry;

        if (!resolve_path(path, entry)) return 0;

        result->inode = entry.inode;
        result->size = entry.size;

        return sizeof(vfs::stat_t);
    }

    size_t read(const char* path, uint8_t* buffer, const uint32_t max_size) {
        vfs::dir_entry entry;

        if (!resolve_path(path, entry))
            return 0;

        uint32_t cluster = static_cast<uint32_t>(entry.inode);
        size_t bytes_read = 0;

        const size_t file_size = entry.size;
        const size_t bytes_to_read = file_size < max_size ? file_size : max_size;

        while (cluster < EOF_MARKER && bytes_read < bytes_to_read) {
            const uint32_t first_sector_of_cluster = calculate_lba(volume, cluster);

            const uint8_t* cluster_data = static_cast<const uint8_t*>(volume.module_addr) +
                first_sector_of_cluster * volume.bytes_per_sector;

            const size_t cluster_size = volume.bytes_per_sector * volume.sectors_per_cluster;
            const size_t remaining = bytes_to_read - bytes_read;

            const size_t bytes_to_copy = remaining < cluster_size ? remaining : cluster_size;

            memcpy(buffer + bytes_read, cluster_data, bytes_to_copy);

            bytes_read += bytes_to_copy;

            if (bytes_read >= bytes_to_read) break;

            cluster = read_fat_entry(
                volume.module_addr,
                volume.first_fat_sector,
                volume.bytes_per_sector,
                cluster
            );
        }

        return bytes_read;
    }

    size_t readdir(const char* path, vfs::dir_entry* entries, const uint32_t max_entries) {
        uint32_t cluster = volume.root_cluster;

        if (strcmp(path, "/") != 0) {
            // walk each path component
            char component[256];
            const char* p = path;

            while (next_component(&p, component)) {
                vfs::dir_entry entry;

                if (!lookup(cluster, component, entry)) {
                    return 0;
                }

                if (entry.type != vfs::file_type::Directory)
                    return 0;

                cluster = static_cast<uint32_t>(entry.inode);
            }
        }

        return read_directory(cluster, entries, max_entries);
    }

    bool lookup(const uint32_t directory_cluster, const char* name, vfs::dir_entry& result) {
        vfs::dir_entry entries[64];
        logger.debug("Looking for '%s' in cluster %u", name, directory_cluster);
        const size_t count = read_directory(directory_cluster, entries, sizeof(entries) / sizeof(vfs::dir_entry));

        for (size_t i = 0; i < count; i++) {
            logger.debug("Comparing: '%s' with '%s'", entries[i].name, name);
            if (strcmp(entries[i].name, name) == 0) {
                result = entries[i];
                return true;
            }
        }

        return false;
    }

    void init(const void* module_addr) {
        const auto* fat_boot = static_cast<const bpb*>(module_addr);
        const auto* ext = reinterpret_cast<const ebr_32*>(static_cast<const uint8_t*>(module_addr) + sizeof(
            bpb));
        const auto* fs_info = reinterpret_cast<const fs_info_t*>(static_cast<const uint8_t*>(module_addr) + sizeof
            (bpb) +
            sizeof(ebr_32));

        if (fs_info->lead_signature != 0x41615252) {
            logger.warn("Unexpected lead signature in FS Info: 0x%X", fs_info->lead_signature);
        }

        if (fs_info->signature != 0x61417272) {
            logger.warn("Unexpected signature in FS Info: 0x%X", fs_info->signature);
        }

        if (fs_info->trail_signature != 0xAA550000) {
            logger.warn("Unexpected trail signature in FS Info: 0x%X", fs_info->trail_signature);
        }

        const uint32_t root_dir_sectors = ((fat_boot->root_entries * 32) + (fat_boot->bytes_per_sector - 1)) / fat_boot
            ->bytes_per_sector;

        const uint32_t total_sectors = fat_boot->sectors != 0 ? fat_boot->sectors : fat_boot->large_sector_count;
        const uint32_t fat_size = ext->sectors_per_fat; // FAT32 always uses this; table_size_16 is always 0 here

        const uint32_t data_sectors = total_sectors - (fat_boot->reserved_sectors + (fat_boot->fats * fat_size) +
            root_dir_sectors);
        const uint32_t total_clusters = data_sectors / fat_boot->sectors_per_cluster;

        const uint32_t first_data_sector = fat_boot->reserved_sectors + (fat_boot->fats * fat_size);
        const uint32_t first_fat_sector = fat_boot->reserved_sectors;

        uint32_t sectorsize = 0;

        fat_type type;

        if (total_clusters < 4085) {
            type = FAT12;
        } else if (total_clusters < 65525) {
            type = FAT16;
        } else {
            type = FAT32;
        }

        if (ext->signature != 0x29 && ext->signature != 0x28) {
            logger.warn("Unexpected extended boot record signature: 0x%X", ext->signature);
        }

        volume.module_addr = module_addr;
        volume.bytes_per_sector = fat_boot->bytes_per_sector;
        volume.entries_per_cluster = (fat_boot->bytes_per_sector * fat_boot->sectors_per_cluster) / sizeof(
            dir_entry);
        volume.sectors_per_cluster = fat_boot->sectors_per_cluster;
        volume.first_data_sector = first_data_sector;
        volume.first_fat_sector = first_fat_sector;
        volume.root_cluster = ext->root_cluster;
    }
}
