#include "driver/pci/pci.hpp"
#include <driver/acpi/acpi.hpp>
#include <driver/block/ahci.hpp>
#include <driver/block/ide.hpp>
#include <uacpi/tables.h>
#include "kernel/log.hpp"
#include "kernel/system.hpp"

namespace pci {
    constexpr uint32_t CONFIG_ADDR = 0xCF8;
    constexpr uint32_t CONFIG_DATA = 0xCFC;

    struct DeviceId {
        uint16_t id;
        const char* name;
    };

    constexpr Vendor vendor_list[] = {
        {0x8086, "Intel"},
        {0x10DE, "NVIDIA"},
        {0x1002, "AMD"},
        {0x10EC, "Realtek Semiconductor Co., Ltd."},
        {0x1234, "QEMU"}
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

    struct SubclassName {
        uint8_t value;
        const char* name;
    };

    constexpr DeviceKey device_map[] = {
        {.vendor = 0x8086, .device = 0x100E, .name = "Intel PRO/1000 (e1000)"},
        {.vendor = 0x8086, .device = 0x1237, .name = "Intel PIIX3 ISA"},
        {.vendor = 0x10DE, .device = 0x1B80, .name = "NVIDIA GeForce GTX 1080"},
        {.vendor = 0x1002, .device = 0x67DF, .name = "AMD Radeon RX 580"},
        {.vendor = 0x8086, .device = 0x2930, .name = "82801I (ICH9 Family) SMBus Controller"}
        // add entries as needed
    };

    constexpr ClassCode class_map[] = {
        {.class_code = 0x00, .name = "Unclassified"},
        {.class_code = 0x01, .name = "Mass Storage Controller"},
        {.class_code = 0x02, .name = "Network Controller"},
        {.class_code = 0x03, .name = "Display Controller"},
        {.class_code = 0x04, .name = "Multimedia Controller"},
        {.class_code = 0x05, .name = "Memory Controller"},
        {.class_code = 0x06, .name = "Bridge"},
        {.class_code = 0x07, .name = "Simple Communication Controller"},
        {.class_code = 0x08, .name = "Base System Peripheral"},
        {.class_code = 0x09, .name = "Input Device Controller"},
        {.class_code = 0x0A, .name = "Docking Station"},
        {.class_code = 0x0B, .name = "Processor"},
        {.class_code = 0x0C, .name = "Serial Bus Controller"},
        {.class_code = 0x0D, .name = "Wireless Controller"},
        {.class_code = 0x0E, .name = "Intelligent Controller"},
        {.class_code = 0x0F, .name = "Satellite Communication Controller"},
        {.class_code = 0x10, .name = "Encryption/Decryption Controller"},
        {.class_code = 0x11, .name = "Signal Processing Controller"},
        {.class_code = 0xFF, .name = "Unassigned Class (Vendor specific)"}
    };

    static constexpr const DeviceKey* device_name(const uint16_t vendor, const uint16_t device) {
        for (const auto& d : device_map) {
            if (d.vendor == vendor && d.device == device) return &d;
        }
        return nullptr;
    }

    static constexpr const Vendor* vendor_name(const uint16_t id) {
        for (const auto& v : vendor_list) {
            if (v.id == id) return &v;
        }
        return nullptr;
    }

    static constexpr const ClassCode* class_name(const uint16_t code) {
        for (const auto& c : class_map) {
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

    uint8_t read_conf8(const uint8_t bus, const uint8_t slot, const uint8_t func, const uint8_t offset) {
        const uint32_t val32 = read_conf32(bus, slot, func, offset);
        const auto byte = static_cast<uint8_t>((val32 >> ((offset & 3) * 8)) & 0xFF);
        return byte;
    }

    void* read_bar(const PciDevice* device, uint8_t bar_index) {
    }

    uint32_t write_conf32(
        const uint8_t bus,
        const uint8_t slot,
        const uint8_t func,
        const uint8_t offset,
        const uint32_t data
    ) {
        const uint32_t address =
            static_cast<uint32_t>(bus) << 16 |
            static_cast<uint32_t>(slot) << 11 |
            static_cast<uint32_t>(func) << 8 |
            (offset & 0xFC) | 0x80000000u;
        outl(CONFIG_ADDR, address);
        outl(CONFIG_DATA, data);
        return data;
    }

    uint16_t write_conf16(
        const uint8_t bus,
        const uint8_t slot,
        const uint8_t func,
        const uint8_t offset,
        const uint16_t data
    ) {
        return write_conf32(bus, slot, func, offset, data);
    }

    uint8_t write_conf8(
        const uint8_t bus,
        const uint8_t slot,
        const uint8_t func,
        const uint8_t offset,
        const uint8_t data
    ) {
        return write_conf32(bus, slot, func, offset, data);
    }

    static uint32_t get_header_type(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t word = read_conf16(bus, slot, func, 0x0E);
        return static_cast<uint8_t>(word & 0xFF);;
    }

    static uint32_t get_vendor_id(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t low = read_conf16(bus, slot, func, 0); // vendor id (offset 0)
        const uint16_t high = read_conf16(bus, slot, func, 2); // device id (offset 2)
        return (static_cast<uint32_t>(high) << 16) | low;
    }

    static uint8_t get_class_code(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t word = read_conf16(bus, slot, func, 0x0A); // reads bytes 0x0A/0x0B
        return static_cast<uint8_t>((word >> 8) & 0xFF); // high byte is class code (0x0B)
    }

    static uint8_t get_subclass_code(const uint8_t bus, const uint8_t slot, const uint8_t func) {
        const uint16_t word = read_conf16(bus, slot, func, 0x0A); // reads bytes 0x0A/0x0B
        return static_cast<uint8_t>(word & 0xFF); // low byte is subclass code (0x0A)
    }

    void enumerate_busses() {
        logger.debug("PCI Devices:");

        uacpi_table* mcfg_table = acpi::find_table("MCFG");

        if (!mcfg_table) {
            logger.error("MCFG table not found. PCI enumeration may be limited.");
        } else {
            logger.debug("MCFG table found at physical address: 0x%lx", reinterpret_cast<uintptr_t>(mcfg_table->ptr));
        }

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

                    const uint8_t subclass_code = get_subclass_code(bus, slot, function);
                    // const SubclassCode* subcls = subclass_name(subclass_code);
                    // const char* subclass_str = subcls ? subcls->name : "Unknown";

                    PciDevice pci_device{};
                    pci_device.bus = bus;
                    pci_device.slot = slot;
                    pci_device.function = function;
                    pci_device.device_id = dev_id;
                    pci_device.class_code.class_code = class_code;
                    pci_device.class_code.name = class_str;
                    pci_device.subclass_code.subclass_code = subclass_code;
                    // pci_device.subclass_code.name = subclass_str;
                    pci_device.vendor.id = vendor_id;
                    pci_device.vendor.name = vendor_str;

                    pci_devices[pci_device_count++] = pci_device;

                    logger.debug(
                        "%x:%x.%x cls=%s subcls=0x%02X vendor=%s device=%04X",
                        pci_device.bus,
                        pci_device.slot,
                        pci_device.function,
                        pci_device.class_code.name,
                        pci_device.subclass_code.subclass_code,
                        pci_device.vendor.name,
                        pci_device.device_id
                    );
                }
            }
        }

        ide::init();
        ahci::init();
    }

    PciDevice* find_device(const uint16_t vendor_id, const uint16_t device_id) {
        for (uint16_t i = 0; i < pci_device_count; ++i) {
            if (pci_devices[i].vendor.id == vendor_id && pci_devices[i].device_id == device_id) {
                return &pci_devices[i];
            }
        }

        return nullptr;
    }
}
