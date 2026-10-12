#include <cstdint>
#include <driver/fs/ext2.hpp>

namespace fs::ext2 {
    /**
     * Contains all info about the layout of the filesystem
     */
    struct super_block {
        uint32_t s_inodes_count;
        uint32_t s_blocks_count;
        uint32_t s_r_blocks_count;
        uint32_t s_free_blocks_count;
        uint32_t s_free_inodes_count;
        uint32_t s_first_data_block;
        uint32_t s_log_block_size;
        uint32_t s_log_frag_size;
        uint32_t s_blocks_per_group;
        uint32_t s_frags_per_group;
        uint32_t s_inodes_per_group;
        uint32_t s_mtime;
        uint32_t s_wtime;
        uint16_t s_mnt_count;
        uint16_t s_max_mnt_count;
        uint16_t s_magic;
        uint16_t s_state;
        uint16_t s_errors;
        uint16_t s_minor_rev_level;
        uint32_t s_lastcheck;
        uint32_t s_checkinterval;
        uint32_t s_creator_os;
        uint32_t s_rev_level;
        uint16_t s_def_resuid;
        uint16_t s_def_resgid;
    };

    struct block_group_desc {
        uint32_t bg_block_bitmap;
        uint32_t bg_inode_bitmap;
        uint32_t bg_inode_table;
        uint16_t bg_free_blocks_count;
        uint16_t bg_free_inodes_count;
        uint16_t bg_used_dirs_count;
        uint16_t bg_pad;
        uint32_t bg_reserved[3];
    };

    struct inode {
    };

    enum os_id {
        LINUX = 0,
        HURD = 1,
        MASIX = 2,
        FREEBSD = 3,
        OTHER = 4
    };

    enum file_type {
        UNKNOWN = 0,
        REGULAR_FILE = 1,
        DIRECTORY = 2,
        CHARACTER_DEVICE = 3,
        BLOCK_DEVICE = 4,
        FIFO = 5,
        SOCKET = 6,
        SYMBOLIC_LINK = 7
    };

    struct directory_entry {
        uint32_t inode;
        uint16_t rec_len;
        uint8_t name_len;
        uint8_t file_type;
        char name[];
    };

    void find_superblock() {
    }

    void init() {
    }

    void Ext2Filesystem::init() {
        find_superblock();
    }
}
