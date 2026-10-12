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

    struct SubclassCode {
        uint16_t subclass_code;
        const char* name;
    };

    struct PciDevice {
        uint8_t bus;
        uint8_t slot;
        uint8_t function;
        uint16_t device_id;
        ClassCode class_code;
        SubclassCode subclass_code;
        Vendor vendor;
    };

    constexpr uint16_t PCI_MAX_DEVICES = 256;
    inline PciDevice pci_devices[PCI_MAX_DEVICES] = {};
    inline uint16_t pci_device_count = 0;

    uint32_t read_conf32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    uint16_t read_conf16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    uint8_t read_conf8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    void* read_bar(const PciDevice* device, uint8_t bar_index);

    uint32_t write_conf32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);
    uint16_t write_conf16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);
    uint8_t write_conf8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data);

    void enumerate_busses();
    PciDevice* find_device(uint16_t vendor_id, uint16_t device_id);
}
