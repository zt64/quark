#pragma once
#include <cstdint>

namespace pci {
    struct Vendor {
        uint16_t id;
        const char* name;
    };

    struct ClassCode {
        uint16_t class_code;
        const char* name;
    };

    struct PciDevice {
        uint8_t bus;
        uint8_t slot;
        uint8_t function;
        uint16_t device_id;
        ClassCode class_code;
        Vendor vendor;
    };

    uint32_t read_conf32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    uint16_t read_conf16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    uint32_t write_conf32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);
    uint16_t write_conf16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);

    void enumerate_busses();
}
