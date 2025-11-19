#include "driver/smbios.hpp"
#include "boot/limine/limine.h"
#include "boot/limine/limine_requests.hpp"
#include "kernel/log.hpp"

namespace smbios {
    namespace {
        enum smbios_structure_type : uint8_t {
            BIOS_INFORMATION = 0,
            SYSTEM_INFORMATION = 1,
            MAINBOARD = 2,
            CHASSIS = 3,
            PROCESSOR = 4,
            CACHE = 7,
            SYSTEM_SLOTS = 9,
            PHYS_MEM_ARRAY = 16,
            MEM_DEVICE = 17,
            MEM_ARR_MAP = 18,
            MEM_DEVICE_MAP = 19,
            SYSTEM_BOOT = 32
        };

        struct smbios_header {
            smbios_structure_type type;
            uint8_t length;
            uint16_t handle;
        };

        struct smbios_structure {
            smbios_header header;

            char** strings;
        };

        struct smbios2_entry_point {
            char anchor[4]; // "_SM_"
            uint8_t checksum;
            uint8_t length;
            uint8_t major_version;
            uint8_t minor_version;
            uint16_t max_structure_size;
            uint8_t entry_point_revision;
            uint8_t formatted_area[5];
            char intermediate_anchor[5]; // "_DMI_"
            uint8_t intermediate_checksum;
            uint16_t table_length;
            uint32_t table_address; // physical address
            uint16_t structure_count;
            uint8_t bcd_revision;
        } __attribute__((packed));

        static_assert(sizeof(smbios2_entry_point) == 0x1F);

        struct smbios3_entry_point {
            char anchor[5]; // "_SM3_"
            uint8_t checksum;
            uint8_t length;
            uint8_t major_version;
            uint8_t minor_version;
            uint8_t doc_revision;
            uint8_t entry_point_revision;
            uint8_t reserved;
            uint32_t table_max_size;
            uint64_t table_address; // physical address
        } __attribute__((packed));

        static_assert(sizeof(smbios3_entry_point) == 0x18);
    }

    static uint32_t smbios_struct_len(smbios_header* hd) {
        uint32_t i;
        const char* strtab = reinterpret_cast<char *>(hd) + hd->length;
        // Scan until we find a double zero byte
        for (i = 1; strtab[i - 1] != '\0' || strtab[i] != '\0'; i++);
        return hd->length + i + 1;
    }

    static const char* smbios_string(const smbios_header* header, const uint8_t index) {
        if (index == 0) return nullptr;

        const char* string = reinterpret_cast<const char *>(header) + header->length;
        for (uint8_t current_index = 1; *string != '\0'; ++current_index) {
            if (current_index == index) return string;

            while (*string != '\0') ++string;
            ++string;
        }

        return nullptr;
    }

    void print() {
        const auto* response = limine_requests::smbios_request.response;
        const uint64_t offset = limine_requests::hhdm_request.response->offset;

        if (response->entry_64 != 0) {
            const auto* entry = reinterpret_cast<const smbios3_entry_point *>(response->entry_64 + offset);

            logger.info(
                "SMBIOS version: %u.%u",
                entry->major_version,
                entry->minor_version
            );
        } else if (response->entry_32 != 0) {
            const auto* entry = reinterpret_cast<const smbios2_entry_point *>(response->entry_32 + offset);

            logger.info(
                "SMBIOS version: %u.%u",
                entry->major_version,
                entry->minor_version
            );

            const auto smbios_table_addr = entry->table_address + offset;

            uint32_t x = 0;
            for (uint16_t i = 0; i < entry->structure_count; ++i) {
                const auto structure_addr = smbios_table_addr + x;
                const auto structure = reinterpret_cast<smbios_header *>(structure_addr);
                const uint32_t length = smbios_struct_len(structure);

                logger.debug(
                    "Header type: %d, length: %d, handle: %d",
                    structure->type,
                    length,
                    structure->handle
                );

                // TODO: Make a way to store strings in a struct
                const auto* fields = reinterpret_cast<const uint8_t *>(structure);
                switch (structure->type) {
                    case BIOS_INFORMATION: {
                        logger.info("BIOS Information");

                        const char* vendor = smbios_string(structure, fields[4]);
                        const char* version = smbios_string(structure, fields[5]);

                        logger.info("Vendor: %s", vendor ? vendor : "<not specified>");
                        logger.info("Version: %s", version ? version : "<not specified>");

                        break;
                    }
                    case SYSTEM_INFORMATION: {
                        logger.info("System Information");

                        const char* manufacturer = smbios_string(structure, fields[4]);
                        const char* product_name = smbios_string(structure, fields[5]);

                        logger.info("Manufacturer: %s", manufacturer ? manufacturer : "<not specified>");
                        logger.info("Product Name: %s", product_name ? product_name : "<not specified>");
                    }
                    case MAINBOARD: {
                        const char* manufacturer = smbios_string(structure, fields[4]);
                        const char* product = smbios_string(structure, fields[5]);
                        const char* version = smbios_string(structure, fields[6]);
                        const char* serial_number = smbios_string(structure, fields[7]);
                        const char* asset_tag = smbios_string(structure, fields[8]);
                        const char* feature_flags = smbios_string(structure, fields[9]);
                        const char* chassis_location = smbios_string(structure, fields[10]);

                        logger.info(
                            "Baseboard Information\n"
                            "Manufacturer: %s\n"
                            "Product: %s\n"
                            "Version: %s\n"
                            "Serial Number: %s\n"
                            "Asset Tag: %s\n"
                            "Feature Flags: %s\n"
                            "Chassis Location: %s\n",
                            manufacturer ? manufacturer : "<not specified>",
                            product ? product : "<not specified>",
                            version ? version : "<not specified>",
                            serial_number ? serial_number : "<not specified>",
                            asset_tag ? asset_tag : "<not specified>",
                            feature_flags ? feature_flags : "<not specified>",
                            chassis_location ? chassis_location : "<not specified>"
                        );
                    }
                    case CHASSIS: {
                        const char* manufacturer = smbios_string(structure, fields[4]);
                        const char* type = smbios_string(structure, fields[5]);
                        const char* version = smbios_string(structure, fields[6]);
                        const char* serial_number = smbios_string(structure, fields[7]);
                        const char* asset_tag = smbios_string(structure, fields[8]);
                        const char* bootup_state = smbios_string(structure, fields[9]);
                        const char* psu_state = smbios_string(structure, fields[10]);
                        const char* thermal_state = smbios_string(structure, fields[11]);
                        const char* security_state = smbios_string(structure, fields[12]);

                        logger.info(
                            "Chassis information\n"
                            "Manufacturer: %s\n"
                            "Product: %s\n"
                            "Version: %s\n"
                            "Serial Number: %s\n"
                            "Asset Tag: %s\n"
                            "Bootup State: %s\n"
                            "PSU State: %s\n"
                            "Thermal State: %s\n"
                            "Security State: %s\n",
                            manufacturer ? manufacturer : "<not specified>",
                            type ? type : "<not specified>",
                            version ? version : "<not specified>",
                            serial_number ? serial_number : "<not specified>",
                            asset_tag ? asset_tag : "<not specified>",
                            bootup_state ? bootup_state : "<not specified>",
                            psu_state ? psu_state : "<not specified>",
                            thermal_state ? thermal_state : "<not specified>",
                            security_state ? security_state : "<not specified>"
                        );
                        break;
                    }
                    case PROCESSOR: {
                        const char* socket_designation = smbios_string(structure, fields[4]);
                        const char* processor_type = smbios_string(structure, fields[5]);
                        const char* processor_family = smbios_string(structure, fields[6]);
                        const char* processor_manufacturer = smbios_string(structure, fields[7]);
                        const char* processor_id = smbios_string(structure, fields[8]);
                        const char* processor_version = smbios_string(structure, fields[9]);
                        const char* voltage = smbios_string(structure, fields[10]);
                        break;
                    }
                    case CACHE:

                        break;
                    case SYSTEM_SLOTS:
                        break;
                    case PHYS_MEM_ARRAY:
                        break;
                    case MEM_DEVICE:
                        break;
                    case MEM_ARR_MAP:
                        break;
                    case MEM_DEVICE_MAP:
                        break;
                    case SYSTEM_BOOT:
                        break;
                }

                x += length;
            }
        } else {
            logger.warn("No SMBIOS entry point available");
        }
    }
}
