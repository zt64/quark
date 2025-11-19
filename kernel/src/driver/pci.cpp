#include "driver/pci.hpp"

#include "driver/serial.hpp"
#include "kernel/system.hpp"
#include "kernel/log.hpp"

constexpr uint8_t CONFIG_ADDR = 0xCF8;
constexpr uint8_t CONFIG_DATA = 0xCFC;
constexpr uint8_t PCI_MAX_DEVICES = 256;

namespace pci {
    PciDevice pci_devices[PCI_MAX_DEVICES] = {};
    uint8_t pci_device_count = 0;

    struct DeviceId {
        uint16_t id;
        const char* name;
    };

    constexpr Vendor vendor_list[] = {
        {0x8086, "Intel"},
        {0x10DE, "NVIDIA"},
        {0x1002, "AMD"},
    };

    struct ProgIfName {
        uint8_t value;
        const char* name;
    };

    struct DeviceKey {
        uint16_t vendor;
        uint16_t device;
        const char* name;
    };

    constexpr DeviceKey device_map[] = {
        {0x8086, 0x100E, "Intel PRO/1000 (e1000)"},
        {0x8086, 0x1237, "Intel PIIX3 ISA"},
        {0x10DE, 0x1B80, "NVIDIA GeForce GTX 1080"},
        {0x1002, 0x67DF, "AMD Radeon RX 580"},
        // add entries as needed
    };

    constexpr ClassCode class_map[] = {
        {0x00, "Unclassified"},
        {0x01, "Mass Storage Controller"},
        {0x02, "Network Controller"},
        {0x03, "Display Controller"},
        {0x04, "Multimedia Controller"},
        {0x05, "Memory Controller"},
        {0x06, "Bridge"},
        {0x07, "Simple Communication Controller"},
        {0x08, "Base System Peripheral"},
        {0x09, "Input Device Controller"},
        {0x0A, "Docking Station"},
        {0x0B, "Processor"},
        {0x0C, "Serial Bus Controller"},
        {0x0D, "Wireless Controller"},
        {0x0E, "Intelligent Controller"}
    };

    constexpr const DeviceKey* device_name(const uint16_t vendor, const uint16_t device) {
        for (const auto&d: device_map) {
            if (d.vendor == vendor && d.device == device) return &d;
        }
        return nullptr;
    }

    constexpr const Vendor* vendor_name(const uint16_t id) {
        for (const auto&v: vendor_list) {
            if (v.id == id) return &v;
        }
        return nullptr;
    }

    constexpr const ClassCode* class_name(const uint16_t code) {
        for (const auto&c: class_map) {
            if (c.class_code == code) return &c;
        }
        return nullptr;
    }

    uint32_t read_conf32(const uint8_t bus, const uint8_t slot, const uint8_t func, const uint8_t offset) {
        const uint32_t address =
                static_cast<uint32_t>(bus) << 16 |
                static_cast<uint32_t>(slot) << 11 |
                static_cast<uint32_t>(func) << 8 |
                (offset & 0xFC) | 0x80000000u;
        outl(CONFIG_ADDR, address);
        return inl(CONFIG_DATA);
    }

    uint16_t read_conf16(const uint8_t bus, const uint8_t slot, const uint8_t func, const uint8_t offset) {
        const uint32_t val32 = read_conf32(bus, slot, func, offset);
        const auto word = static_cast<uint16_t>((val32 >> ((offset & 2) ? 16 : 0)) & 0xFFFF);
        return word;
    }

    uint32_t write_conf32(
        const uint8_t bus,
        const uint8_t slot,
        const uint8_t func,
        const uint8_t offset,
        const uint32_t data
    ) {
    }

    uint16_t write_conf16(
        const uint8_t bus,
        const uint8_t slot,
        const uint8_t func,
        const uint8_t offset,
        const uint16_t data
    ) {
    }

    static uint32_t get_header_type(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t word = read_conf16(bus, slot, func, 0x0E);
        const auto header = static_cast<uint8_t>(word & 0xFF);
        return header;
    }

    uint32_t get_vendor_id(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t low = read_conf16(bus, slot, func, 0); // vendor id (offset 0)
        const uint16_t high = read_conf16(bus, slot, func, 2); // device id (offset 2)
        return (static_cast<uint32_t>(high) << 16) | low;
    }

    uint8_t get_class_code(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t word = read_conf16(bus, slot, func, 0x0A); // reads bytes 0x0A/0x0B
        return static_cast<uint8_t>((word >> 8) & 0xFF); // high byte is class code (0x0B)
    }

    void enumerate_busses() {
        logger.debug("PCI Devices:");

        for (uint16_t bus = 0; bus < 256; bus++) {
            for (uint8_t slot = 0; slot < 32; slot++) {
                const auto header0 = static_cast<uint8_t>(get_header_type(bus, slot, 0) & 0xFF);
                const uint8_t max_functions = (header0 & 0x80) ? 8 : 1;

                for (uint8_t function = 0; function < max_functions; ++function) {
                    const uint32_t vendor_dev = get_vendor_id(bus, slot, function);
                    const auto vendor_id = static_cast<uint16_t>(vendor_dev & 0xFFFF);
                    if (vendor_id == 0xFFFF) continue; // no device present

                    const auto dev_id = static_cast<uint16_t>((vendor_dev >> 16) & 0xFFFF);
                    const Vendor* vendor = vendor_name(vendor_id);
                    const char* vendor_str = vendor ? vendor->name : "Unknown";

                    const uint8_t class_code = get_class_code(bus, slot, function);
                    const ClassCode* cls = class_name(class_code);
                    const char* class_str = cls ? cls->name : "Unknown";

                    PciDevice pci_device{};
                    pci_device.bus = bus;
                    pci_device.slot = slot;
                    pci_device.function = function;
                    pci_device.device_id = dev_id;
                    pci_device.class_code.class_code = class_code;
                    pci_device.class_code.name = class_str;
                    pci_device.vendor.id = vendor_id;
                    pci_device.vendor.name = vendor_str;

                    pci_devices[pci_device_count++] = pci_device;

                    logger.debug(
                        "%x:%x.%x cls=%s vendor=%s device=%04X",
                        pci_device.bus,
                        pci_device.slot,
                        pci_device.function,
                        pci_device.class_code.name,
                        pci_device.vendor.name,
                        pci_device.device_id
                    );
                }
            }
        }
    }
}
