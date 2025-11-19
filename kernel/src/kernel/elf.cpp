#include "kernel/elf.hpp"

#include <cstdint>
#include <cstring>
#include <lib/math.hpp>

#include "kernel/log.hpp"
#include "kernel/system.hpp"
#include "memory/paging.hpp"
#include "memory/pmm.hpp"
#include "memory/vmm.hpp"

// https://0xc0ffee.netlify.app/osdev/21-elf-loader-p1#elf-reader

namespace elf {
    typedef uint64_t Elf64_Addr;
    typedef uint16_t Elf64_Half;
    typedef int16_t Elf64_SHalf;
    typedef uint64_t Elf64_Off;
    typedef int32_t Elf64_Sword;
    typedef uint32_t Elf64_Word;
    typedef uint64_t Elf64_Xword;
    typedef int64_t Elf64_Sxword;

    constexpr uint8_t ELF_NIDENT = 16;

    constexpr uint8_t ELFMAG0 = 0x7F; // e_ident[EI_MAG0]
    constexpr uint8_t ELFMAG1 = 'E'; // e_ident[EI_MAG1]
    constexpr uint8_t ELFMAG2 = 'L'; // e_ident[EI_MAG2]
    constexpr uint8_t ELFMAG3 = 'F'; // e_ident[EI_MAG3]

    constexpr uint8_t ELFDATA2LSB = 1; // Little Endian
    constexpr uint8_t ELFCLASS32 = 1; // 32-bit Architecture
    constexpr uint8_t ELFCLASS64 = 2; // 64-bit Architecture

    namespace {
        enum Elf_Ident : uint8_t {
            EI_MAG0 = 0, // 0x7F
            EI_MAG1 = 1, // 'E'
            EI_MAG2 = 2, // 'L'
            EI_MAG3 = 3, // 'F'
            EI_CLASS = 4, // Architecture (32/64)
            EI_DATA = 5, // Byte Order
            EI_VERSION = 6, // ELF Version
            EI_OSABI = 7, // OS Specific
            EI_ABIVERSION = 8, // OS Specific
            EI_PAD = 9 // Padding
        };

        enum Elf_Type : Elf64_Half {
            ET_NONE = 0, // Unkown Type
            ET_REL = 1, // Relocatable File
            ET_EXEC = 2 // Executable File
        };

        typedef struct {
            Elf_Ident ident[ELF_NIDENT];
            Elf_Type type;
            Elf64_Half machine;
            Elf64_Word version;
            Elf64_Addr entry;
            Elf64_Off phoff;
            Elf64_Off shoff;
            Elf64_Word flags;
            Elf64_Half ehsize;
            Elf64_Half phentsize;
            Elf64_Half phnum;
            Elf64_Half shentsize;
            Elf64_Half shnum;
            Elf64_Half shstrndx;
        } ElfHeader;

        enum ElfProgramHeaderType : Elf64_Word {
            NULL_T = 0,
            LOAD = 1,
            DYNAMIC = 2,
            INTERP = 3,
            NOTE = 4,
            SHLIB = 5,
            PHDR = 6,
            TLS = 7
        };

        typedef struct {
            ElfProgramHeaderType p_type;
            Elf64_Word p_flags;
            Elf64_Off p_offset;
            Elf64_Addr p_vaddr;
            Elf64_Addr p_paddr;
            Elf64_Xword p_filesz;
            Elf64_Xword p_memsz;
            Elf64_Xword p_align;
        } ElfProgramHeader;

#define ELF32_ST_BIND(INFO)	((INFO) >> 4)
#define ELF32_ST_TYPE(INFO)	((INFO) & 0x0F)

# define ELF64_R_SYM(INFO)	((INFO) >> 32)
# define ELF64_R_TYPE(INFO)	((uint32_t)(INFO))
    }

    constexpr uint8_t EM_386 = 3; // x86 Machine Type
    constexpr uint8_t EM_X86_64 = 62; // x86_64 Machine Type
    constexpr uint8_t EV_CURRENT = 1; // ELF Current Version

    static bool elf_check_file(const ElfHeader* hdr) {
        if (!hdr) return false;
        if (hdr->ident[EI_MAG0] != ELFMAG0) {
            logger.error("ELF Header EI_MAG0 incorrect.\n");
            return false;
        }
        if (hdr->ident[EI_MAG1] != ELFMAG1) {
            logger.error("ELF Header EI_MAG1 incorrect.\n");
            return false;
        }
        if (hdr->ident[EI_MAG2] != ELFMAG2) {
            logger.error("ELF Header EI_MAG2 incorrect.\n");
            return false;
        }
        if (hdr->ident[EI_MAG3] != ELFMAG3) {
            logger.error("ELF Header EI_MAG3 incorrect.\n");
            return false;
        }
        return true;
    }

    static bool elf_check_supported(const ElfHeader* hdr) {
        if (!elf_check_file(hdr)) {
            logger.error("Invalid ELF File.\n");
            return false;
        }
        if (hdr->ident[EI_CLASS] != ELFCLASS64) {
            logger.error("Unsupported ELF File Class.\n");
            return false;
        }
        if (hdr->ident[EI_DATA] != ELFDATA2LSB) {
            logger.error("Unsupported ELF File byte order.\n");
            return false;
        }
        if (hdr->machine != EM_X86_64) {
            logger.error("Unsupported ELF File target.\n");
            return false;
        }
        if (hdr->ident[EI_VERSION] != EV_CURRENT) {
            logger.error("Unsupported ELF File version.\n");
            return false;
        }
        if (hdr->type != ET_REL && hdr->type != ET_EXEC) {
            logger.error("Unsupported ELF File type.\n");
            return false;
        }
        return true;
    }

#define ELF_RELOC_ERR (-1)

    ElfImage* load_file(uintptr_t cr3, const uintptr_t addr) {
        const auto* hdr = reinterpret_cast<const ElfHeader*>(addr);
        if (!elf_check_supported(hdr)) {
            logger.error("ELF File cannot be loaded.\n");
            return nullptr;
        }
        switch (hdr->type) {
            case ET_EXEC: {
                const auto img = new ElfImage();
                img->segment_count = 0;
                img->base_address = UINT32_MAX;
                img->image_size = 0;
                uintptr_t max_end = 0;
                for (uint32_t i = 0; i < hdr->phnum; i++) {
                    const auto phdr = reinterpret_cast<const ElfProgramHeader*>(
                        reinterpret_cast<const uint8_t*>(hdr) + hdr->phoff)[i];

                    if (phdr.p_type == LOAD) {
                        if (phdr.p_align != paging::PAGE_SIZE) {
                            panic("ELF File segment %u is not page-aligned.\n", i);
                        }
                        const uint32_t start_offset = phdr.p_vaddr & (paging::PAGE_SIZE - 1);
                        const uint32_t start_page = phdr.p_vaddr - start_offset;
                        const uint32_t num_pages = (start_offset + phdr.p_memsz + (paging::PAGE_SIZE - 1))
                            / paging::PAGE_SIZE;

                        img->base_address = min(img->base_address, start_page);
                        max_end = max(max_end, start_page + num_pages * paging::PAGE_SIZE);

                        void* page_phys = mem::allocate_physical_pages(num_pages);
                        const auto page_virt = reinterpret_cast<void*>(
                            reinterpret_cast<uintptr_t>(page_phys) + paging::g_hhdm_offset
                        );

                        memset(page_virt, 0, num_pages * paging::PAGE_SIZE);

                        segment seg = {};
                        seg.virt_base = start_page;
                        seg.phys_base = reinterpret_cast<uintptr_t>(page_phys);
                        seg.page_count = num_pages;

                        img->segments[img->segment_count++] = seg;
                        for (uint32_t p = 0; p < num_pages; p++) {
                            paging::map_page(
                                cr3,
                                start_page + p * paging::PAGE_SIZE,
                                reinterpret_cast<uintptr_t>(page_phys) + p * paging::PAGE_SIZE,
                                paging::PAGE_PRESENT | paging::PAGE_WRITABLE | paging::PAGE_USER
                            );
                        }

                        memcpy(
                            static_cast<uint8_t*>(page_virt) + start_offset,
                            reinterpret_cast<const uint8_t*>(hdr) + phdr.p_offset,
                            phdr.p_filesz
                        );
                    }
                }

                img->entry_point = reinterpret_cast<void*>(hdr->entry);
                img->image_size = static_cast<uint32_t>(max_end - img->base_address);

                return img;
            }
            case ET_REL:
                logger.error("Relocatable ELF files are not supported.\n");
            default: ;
        }
        return nullptr;
    }

    void unload_file(const ElfImage* image) {
        if (image == nullptr) {
            return;
        }

        for (uint32_t i = 0; i < image->segment_count; ++i) {
            const auto& [virt_base, phys_base, page_count] = image->segments[i];
            for (uint32_t p = 0; p < page_count; ++p) {
                paging::unmap_page(virt_base + p * paging::PAGE_SIZE);
            }
            mem::free_physical_pages(reinterpret_cast<void*>(phys_base), page_count);
        }

        delete image;
    }
}
