#include "arch/x86_64/gdt.hpp"
#include <cstdint>

namespace gdt {
    namespace {
        constexpr uint8_t ACCESS_PRESENT = 0x80;
        constexpr uint8_t ACCESS_RING3 = 0x60; // DPL = 3
        constexpr uint8_t ACCESS_CODE_DATA = 0x10; // "S" bit: code/data, not a system segment
        constexpr uint8_t ACCESS_EXECUTABLE = 0x08;
        constexpr uint8_t ACCESS_RW = 0x02; // readable (code) / writable (data)

        constexpr uint8_t KERNEL_CODE_ACCESS = ACCESS_PRESENT | ACCESS_CODE_DATA | ACCESS_EXECUTABLE | ACCESS_RW;
        // 0x9A
        constexpr uint8_t KERNEL_DATA_ACCESS = ACCESS_PRESENT | ACCESS_CODE_DATA | ACCESS_RW; // 0x92
        constexpr uint8_t USER_CODE_ACCESS = KERNEL_CODE_ACCESS | ACCESS_RING3; // 0xFA
        constexpr uint8_t USER_DATA_ACCESS = KERNEL_DATA_ACCESS | ACCESS_RING3; // 0xF2

        constexpr uint8_t GRAN_64BIT = 0xA0; // long-mode (L) bit + granularity nibble

        struct gdt_entry {
            uint16_t limit_low;
            uint16_t base_low;
            uint8_t base_middle;
            uint8_t access;
            uint8_t granularity;
            uint8_t base_high;
        } __attribute__((packed));

        struct tss_descriptor {
            uint16_t limit_low;
            uint16_t base_low;
            uint8_t base_middle;
            uint8_t access;
            uint8_t granularity;
            uint8_t base_high;
            uint32_t base_upper;
            uint32_t reserved;
        } __attribute__((packed));

        struct gdt_table {
            gdt_entry entries[5];
            tss_descriptor tss;
        };

        gdt_table table;

        struct gdt_ptr {
            uint16_t limit;
            uint64_t base;
        } __attribute__((packed));

        extern "C" void gdt_flush();
    }

    extern "C" gdt_ptr gp;
    gdt_ptr gp;

    namespace {
        // Set up a code/data descriptor in the GDT
        void set_gate(
            const uint32_t num,
            const uint64_t base,
            const uint32_t limit,
            const uint8_t access,
            const uint8_t gran
        ) {
            auto& entry = table.entries[num];

            entry.base_low = base & 0xFFFF;
            entry.base_middle = (base >> 16) & 0xFF;
            entry.base_high = (base >> 24) & 0xFF;
            entry.limit_low = limit & 0xFFFF;
            entry.granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
            entry.access = access;
        }
    }

    // Sets up the GDT (null, kernel code/data, user code/data)
    void init() {
        gp.limit = sizeof(table) - 1;
        gp.base = reinterpret_cast<uintptr_t>(&table);

        set_gate(0, 0, 0, 0x00, 0x00); // null descriptor
        set_gate(1, 0, 0, KERNEL_CODE_ACCESS, GRAN_64BIT); // selector::KERNEL_CS
        set_gate(2, 0, 0, KERNEL_DATA_ACCESS, 0x00); // selector::KERNEL_DS
        set_gate(3, 0, 0, USER_CODE_ACCESS, GRAN_64BIT); // selector::USER_CS (minus RPL)
        set_gate(4, 0, 0, USER_DATA_ACCESS, 0x00); // selector::USER_DS (minus RPL)

        gdt_flush();
    }

    void install_tss(const uintptr_t base, const uint32_t limit) {
        table.tss.limit_low = limit & 0xFFFF;
        table.tss.base_low = base & 0xFFFF;
        table.tss.base_middle = (base >> 16) & 0xFF;
        table.tss.access = 0x89; // present, DPL=0, 64-bit TSS (available)
        table.tss.granularity = (limit >> 16) & 0x0F;
        table.tss.base_high = (base >> 24) & 0xFF;
        table.tss.base_upper = base >> 32;
        table.tss.reserved = 0;
    }
}
