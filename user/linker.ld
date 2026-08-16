OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

/* mlibc's static-PIE rtld path (options/rtld/generic/main.cpp) reads its own
 * ELF header via the linker-provided __ehdr_start symbol. ld only defines
 * that symbol if the file header/program headers are mapped into a loaded
 * segment, which requires FILEHDR PHDRS on the first PT_LOAD segment. */
PHDRS {
    text PT_LOAD FILEHDR PHDRS FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = 0x0000000040000000;

    .text : {
        *(.text .text.*)
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .data : {
        *(.data .data.*)

        /* mlibc's static rtld linker.cpp walks the C++ global constructor /
         * destructor tables via these linker-provided symbols. They are only
         * defined if the corresponding output sections exist. */
        PROVIDE(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE(__preinit_array_end = .);

        PROVIDE(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);

        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE(__fini_array_end = .);
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
