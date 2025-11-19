#include <cstddef>
#include "driver/ps2/keyboard.hpp"
#include "kernel/system.hpp"
#include "driver/pic.hpp"
#include "driver/ps2/ps2.hpp"

#define KB_BUFFER_SIZE  64

static key_listener_t g_listener = nullptr;

static volatile uint8_t kb_buf[KB_BUFFER_SIZE];
static volatile uint8_t kb_buf_head = 0;
static volatile uint8_t kb_buf_tail = 0;

static void kb_enqueue(const uint8_t scancode) {
    if (const size_t next = (kb_buf_head + 1) % sizeof(kb_buf); next != kb_buf_tail) {
        kb_buf[next] = scancode;
        kb_buf_head = next;
    }
}

static bool kb_dequeue(uint8_t* out) {
    if (kb_buf_tail == kb_buf_head) return false;
    *out = kb_buf[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % sizeof(kb_buf);
    return true;
}

namespace keyboard {
    static void kb_handler(const regs* _) {
        const unsigned char scancode = inb(PS2_DATA_PORT);

        kb_enqueue(scancode);
        process_queue();
    }

    void init() {
        pic::unmask_irq(1); // unmask keyboard IRQ (IRQ1)

        irq::install_handler(1, kb_handler);
    }

    void register_listener(const key_listener_t listener) {
        g_listener = listener;
    }

    void unregister_listener() { g_listener = nullptr; }

    bool break_key = false;
    bool ext_key = false;

    void process_queue() {
        uint8_t sc;
        while (kb_dequeue(&sc)) {
            if (sc == 0xE0) {
                ext_key = true;
                continue;
            }

            if (sc == 0xF0) {
                break_key = true;
                continue;
            }

            const KeyEvent ev = {
                .scancode = sc,
                .break_key = break_key,
                .extended_key = ext_key
            };

            break_key = false;
            ext_key = false;

            if (g_listener) g_listener(ev);
        }
    }

    size_t read_events(KeyEvent* buf, const size_t count) {
        if (kb_buf_head == kb_buf_tail) return 0;

        size_t read_count = 0;
        while (read_count < count && kb_dequeue(reinterpret_cast<uint8_t*>(&buf[read_count]))) {
            read_count++;
        }

        kb_buf_head = 0;
        kb_buf_tail = 0;

        return read_count;
    }
}
