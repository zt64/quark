#pragma once

#include <cstdint>

enum Key {
    KEY_ESCAPE = 0x76,
    KEY_SPACE = 0x29,

    KEY_ARROW_UP = 0x75,
    KEY_ARROW_DOWN = 0x72,
    KEY_ARROW_LEFT = 0x6B,
    KEY_ARROW_RIGHT = 0x74,

    KEY_SHIFT_LEFT = 0x12,
    KEY_SHIFT_RIGHT = 0x59,
    KEY_CONTROL = 0x14,

    KEY_A = 0x1C,
    KEY_C = 0x21,
    KEY_D = 0x23,
    KEY_R = 0x2D,
    KEY_P = 0x4D,
    KEY_S = 0x1B,
    KEY_W = 0x1D,
    KEY_X = 0x22,
    KEY_Z = 0x1A,
};

struct KeyEvent {
    uint8_t scancode;
    bool break_key;
    bool extended_key;
};

using key_listener_t = void(*)(KeyEvent ev);

namespace keyboard {
    /**
     * Register interrupt on channel 1 for handling keypresses
     */
    void init();

    void register_listener(key_listener_t listener);

    void unregister_listener();

    /**
     * Process any queued scancodes, forwarding them to the listener
     */
    void process_queue();

    /**
     * Read queued KeyEvents into the buffer
     */
    size_t read_events(KeyEvent* buf, size_t count);
}
