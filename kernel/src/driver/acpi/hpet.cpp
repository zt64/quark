#include <cstdint>
#include <driver/acpi/acpi.hpp>
#include <kernel/log.hpp>

namespace acpi::hpet {
    namespace {
        struct address_structure {
            uint8_t address_space_id; // 0 - system memory, 1 - system I/O
            uint8_t register_bit_width;
            uint8_t register_bit_offset;
            uint8_t reserved;
            uint64_t address;
        } __attribute__((packed));

        struct description_table_header {
            char signature[4]; // 'HPET' in case of HPET table
            uint32_t length;
            uint8_t revision;
            uint8_t checksum;
            char oemid[6];
            uint64_t oem_tableid;
            uint32_t oem_revision;
            uint32_t creator_id;
            uint32_t creator_revision;
        } __attribute__((packed));
    }

    struct hpet : description_table_header {
        uint8_t hardware_rev_id;
        uint8_t comparator_count : 5;
        uint8_t counter_size : 1;
        uint8_t reserved : 1;
        uint8_t legacy_replacement : 1;
        uint16_t pci_vendor_id;
        address_structure address;
        uint8_t hpet_number;
        uint16_t minimum_tick;
        uint8_t page_protection;
    } __attribute__((packed));

    struct capabilities {
        uint8_t rev_id : 8;
        uint8_t timer_num: 5;
        uint8_t counter_size: 1;
        uint8_t reserved: 1;
        uint8_t legacy_replacement: 1;
        uint16_t vendor_id;
        uint32_t counter_clk_period;
    } __attribute__((packed));

    constexpr uint8_t REG_GENERAL_CAPS_ID = 0x00;
    constexpr uint8_t REG_GENERAL_CONFIG = 0x10;
    constexpr uint8_t REG_GENERAL_INT_STATUS = 0x20;
    constexpr uint8_t REG_MAIN_COUNTER = 0xF0;

    void init() {
        const uacpi_table* t = acpi::find_table("HPET");

        if (!t) {
            logger.error("HPET table not found");
            return;
        }
    }
}
