#include "memory/heap.hpp"

#include <cstdint>
#include <cstring>
#include <kernel/system.hpp>
#include <memory/paging.hpp>

constexpr size_t HEAP_SIZE = 64 * 1024 * 1024; // 64 MB
constexpr size_t HEAP_ALIGNMENT = 16;

alignas(HEAP_ALIGNMENT)
static uint8_t heap_buf[HEAP_SIZE];

namespace {
    struct block {
        size_t size;
        bool free;

        block* next;
        block* prev;
    };
}

static block* heap_head = nullptr;

/**
 * Aligns the given value `v` up to the nearest multiple of `align`.
 * @param v The value to align.
 * @param align The alignment boundary.
 * @return The aligned value.
 */
static size_t align_up(const size_t v, const size_t align) {
    return (v + (align - 1)) & ~(align - 1);
}

static void heap_init() {
    heap_head = reinterpret_cast<block*>(heap_buf);

    heap_head->size = HEAP_SIZE - sizeof(block);
    heap_head->free = true;
    heap_head->next = nullptr;
    heap_head->prev = nullptr;
}

static bool heap_initialized = false;

static void ensure_heap_initialized() {
    if (!heap_initialized) {
        heap_init();
        heap_initialized = true;
    }
}

/**
 * Expands the heap by the specified size. If the requested size exceeds the maximum heap size, it triggers a panic.
 * @param size The size to expand the heap by
 */
static void expand_heap(const size_t size) {
    if (size > HEAP_SIZE) {
        panic("Out of memory: requested size %zu exceeds heap size %zu", size, HEAP_SIZE);
    }

    const size_t asize = align_up(size, HEAP_ALIGNMENT);

    const size_t pages = (asize + sizeof(block) + paging::PAGE_SIZE - 1) / paging::PAGE_SIZE;

    for (size_t i = 0; i < pages; i++) {
        const uintptr_t page_addr = reinterpret_cast<uintptr_t>(heap_buf) + i * paging::PAGE_SIZE;
        paging::map_page(
            paging::g_cr3_value,
            page_addr,
            page_addr,
            paging::PAGE_USER | paging::PAGE_WRITABLE
        );
    }
}

static block* find_free_block(const size_t size) {
    block* current = heap_head;

    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return nullptr; // No suitable block found
}

static block* get_current(void* ptr) {
    return static_cast<block*>(ptr) - 1;
}

static void split_block(block* current, const size_t size) {
    if (current->size <= size + sizeof(block) + HEAP_ALIGNMENT) return;

    auto* new_block = reinterpret_cast<block*>(
        reinterpret_cast<uint8_t*>(current + 1) + size
    );

    new_block->size = current->size - size - sizeof(block);
    new_block->free = true;

    new_block->next = current->next;
    new_block->prev = current;

    // update the next block's previous pointer if it exists
    if (current->next) {
        current->next->prev = new_block;
    }

    current->next = new_block;
    current->size = size;
}

static void merge_with_next(block* current) {
    if (current->next && current->next->free) {
        current->size += sizeof(block) + current->next->size;
        current->next = current->next->next;
        if (current->next) {
            current->next->prev = current;
        }
    }
}

/**
 * Merges two adjacent free blocks into a single larger block if both are free.
 * @param block The block to coalesce with its next block.
 */
static void coalesce_block(block* block) {
    if (block->next && block->next->free) {
        merge_with_next(block);
    }

    if (block->prev && block->prev->free) {
        merge_with_next(block->prev);
    }
}

void* kalloc(const size_t size) {
    if (!size) return nullptr;

    ensure_heap_initialized();

    const size_t asize = align_up(size, HEAP_ALIGNMENT);
    block* block = find_free_block(asize);

    if (!block) {
        expand_heap(asize);
        panic("Out of memory: kalloc failed to allocate %zu bytes", size);
    }

    split_block(block, asize);
    block->free = false;

    // return the data portion which is right after the block header
    return block + 1;
}

void kfree(void* ptr) {
    if (!ptr) return; // Already freed

    // returns the header directly before the pointer
    auto* current_block = static_cast<block*>(ptr) - 1;

    if (current_block->free) {
        panic("Double free detected");
    }

    current_block->free = true;

    coalesce_block(current_block);
}

/**
 * Reallocate the memory block pointed to by `ptr` to a new size `size`. If `ptr` is null, it behaves like `kalloc`.
 * If `size` is zero, it behaves like `kfree` and returns nullptr.
 * @param ptr Pointer to the memory block to be reallocated.
 * @param size The new size for the memory block.
 * @return Pointer to the newly allocated memory block, or nullptr if the allocation fails or if size is zero.
 */
void* krealloc(void* ptr, size_t size) {
    // If ptr is null, krealloc behaves like kalloc
    if (!ptr) {
        return kalloc(size);
    }

    // If size is zero, krealloc behaves like kfree and returns nullptr
    if (size == 0) {
        kfree(ptr);
        return nullptr;
    }

    const auto* current = get_current(ptr);

    size = align_up(size, HEAP_ALIGNMENT);

    // If the current block is already large enough, return the same pointer
    if (current->size >= size) {
        return ptr;
    }

    void* new_ptr = kalloc(size);

    if (!new_ptr) {
        return nullptr; // Allocation failed
    }

    memcpy(new_ptr, ptr, current->size);

    kfree(ptr);

    return new_ptr;
}
