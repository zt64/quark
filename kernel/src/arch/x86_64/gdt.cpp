#include <cstdint>
#include "kernel/gdt.hpp"

namespace {
    struct gdt_entry {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_middle;
        uint8_t access;
        uint8_t granularity;
        uint8_t base_high;
    } __attribute__((packed));
}

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

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

gdt_table table;

gdt_ptr gp;

extern "C" void gdt_flush();

namespace gdt {
    /* Set up a descriptor in the Global Descriptor Table */
    static void set_gate(
        const uint32_t num,
        const uint64_t base,
        const uint32_t limit,
        const uint8_t access,
        const uint8_t gran
    ) {
        auto&entry = table.entries[num];

        entry.base_low = base & 0xFFFF;
        entry.base_middle = (base >> 16) & 0xFF;
        entry.base_high = (base >> 24) & 0xFF;
        entry.limit_low = limit & 0xFFFF;
        entry.granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
        entry.access = access;
    }

    /* Should be called by main. This will setup the special GDT
    *  pointer, set up the first 3 entries in our GDT, and then
    *  finally call gdt_flush() in our assembler file in order
    *  to tell the processor where the new GDT is and update the
    *  new segment registers */
    void init() {
        gp.limit = sizeof(table) - 1;
        gp.base = reinterpret_cast<uintptr_t>(&table);

        set_gate(0, 0, 0, 0x00, 0x00);
        set_gate(1, 0, 0, 0x9A, 0xA0);
        set_gate(2, 0, 0, 0x92, 0x00);
        set_gate(3, 0, 0, 0xFA, 0xA0);
        set_gate(4, 0, 0, 0xF2, 0x00);

        gdt_flush();
    }

    void install_tss(const uintptr_t base, const uint32_t limit) {
        table.tss.limit_low = limit & 0xFFFF;
        table.tss.base_low = base & 0xFFFF;
        table.tss.base_middle = (base >> 16) & 0xFF;
        table.tss.access = 0x89;
        table.tss.granularity = (limit >> 16) & 0x0F;
        table.tss.base_high = (base >> 24) & 0xFF;
        table.tss.base_upper = base >> 32;
        table.tss.reserved = 0;
    }
}
