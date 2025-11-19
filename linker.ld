OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kmain)

PHDRS {
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS {
    KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;
    KERNEL_START     = KERNEL_VIRT_BASE;

    . = KERNEL_VIRT_BASE;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        PROVIDE(kernel_text_start = .);
        *(.text .text.*)
        PROVIDE(kernel_text_end = .);
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        PROVIDE(kernel_rodata_start = .);
        *(.rodata .rodata.*)
        PROVIDE(kernel_rodata_end = .);
    } :rodata

    /* Add a .note.gnu.build-id output section in case a build ID flag is added to the */
    /* linker command. */
    .note.gnu.build-id : {
        *(.note.gnu.build-id)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        PROVIDE(kernel_data_start = .);
        *(.data .data.*)
    } :data

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE(kernel_data_end = .);
    } :data

    KERNEL_END = .;
    KERNEL_SIZE = KERNEL_END - KERNEL_START;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }

    PROVIDE(kernel_virt_base = KERNEL_VIRT_BASE);
    PROVIDE(kernel_start = KERNEL_START);
    PROVIDE(kernel_end = KERNEL_END);
    PROVIDE(kernel_size = KERNEL_SIZE);
}