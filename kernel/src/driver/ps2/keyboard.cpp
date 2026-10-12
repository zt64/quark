#include <cstddef>
#include "driver/ps2/keyboard.hpp"
#include "kernel/system.hpp"
#include "driver/irq.hpp"
#include "driver/pic.hpp"
#include "driver/ps2/ps2.hpp"

namespace ps2::keyboard {
    constexpr uint16_t KB_BUFFER_SIZE = 256;
    constexpr uint8_t KEYBOARD_IRQ = 1;
    constexpr uint8_t EXTENDED_KEY = 0xE0;

    constexpr uint8_t CMD_SET_LEDS = 0xED;
    constexpr uint8_t CMD_GET_SET_SCANCODE_SET = 0xF0;
    constexpr uint8_t CMD_SET_RATE = 0xF3;

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

    static void kb_handler(const regs* _) {
        const unsigned char scancode = inb(PS2_DATA_PORT);

        kb_enqueue(scancode);
        process_queue();
    }

    static void send_cmd(const uint8_t cmd) {
        outb(PS2_COMMAND_PORT, cmd);
    }

    void init() {
        pic::unmask_irq(KEYBOARD_IRQ); // unmask keyboard IRQ (IRQ1)

        // send_cmd(CMD_GET_SET_SCANCODE_SET);

        irq::install_handler(KEYBOARD_IRQ, kb_handler);
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
            if (sc == EXTENDED_KEY) {
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
