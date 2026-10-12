#include <cstddef>
#include <driver/graphics/fb.hpp>
#include <kernel/console.hpp>

namespace console {
    namespace {
        struct Cell {
            uint32_t fg_color;
            uint32_t bg_color;
            char character;
        };

        Cell** buffer;
        uint32_t rows, cols;
        uint32_t cursor_x, cursor_y = 0;
    }

    constexpr uint32_t DEFAULT_FG_COLOR = 0xFFFFFF; // White
    constexpr uint32_t DEFAULT_BG_COLOR = 0x000000; // Black
    constexpr uint8_t SCALE = 2;
    constexpr uint8_t CELL_SIZE = 8 * SCALE; // 8x8 pixels per character
    constexpr uint8_t CURSOR_WIDTH = 2; // 2 pixels wide
    constexpr uint8_t CURSOR_HEIGHT = 2;
    constexpr uint32_t CURSOR_COLOR = 0xFFFFFF; // White

    static bool cursor_visible = true;
    static uint64_t last_cursor_tick = 0;

    void render_cursor() {
        if (!initialized) return;

        const uint32_t x = cursor_x * CELL_SIZE;
        const uint32_t y = cursor_y * CELL_SIZE;

        if (cursor_visible) {
            for (uint32_t py = y; py < y + CELL_SIZE; ++py)
                for (uint32_t px = x; px < x + CELL_SIZE; ++px)
                    fb::put_pixel(px, py, CURSOR_COLOR);
        } else {
            const char ch = buffer[cursor_y][cursor_x].character;
            fb::draw_char(ch, x, y, SCALE,
                          buffer[cursor_y][cursor_x].fg_color,
                          buffer[cursor_y][cursor_x].bg_color);
        }

        fb::flush();
    }

    void tick_cursor() {
        // const uint64_t now = timer::now_ms();
        // if (now - last_cursor_tick >= 500) {
        //     last_cursor_tick = now;
        //     cursor_visible = !cursor_visible;
        //     render_cursor();
        // }
    }

    void init() {
        rows = fb::framebuffer.height / CELL_SIZE;
        cols = fb::framebuffer.width / CELL_SIZE;

        buffer = new Cell*[rows];
        for (uint32_t i = 0; i < rows; ++i) {
            buffer[i] = new Cell[cols];
            for (uint32_t j = 0; j < cols; ++j) {
                buffer[i][j] = {.fg_color = DEFAULT_FG_COLOR, .bg_color = DEFAULT_BG_COLOR, .character = ' '};
            }
        }

        clear();

        initialized = true;
    }

    void scroll() {
    }

    static void putchar(const char c) {
        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= rows) {
                scroll();
                cursor_y = rows - 1;
            }
        } else if (c == '\r') {
            cursor_x = 0;
        } else if (c == '\b') {
            if (cursor_x > 0) {
                cursor_x--;
                buffer[cursor_y][cursor_x] = {
                    .fg_color = DEFAULT_FG_COLOR, .bg_color = DEFAULT_BG_COLOR, .character = ' '
                };
                fb::draw_char(' ', cursor_x * CELL_SIZE, cursor_y * CELL_SIZE, SCALE, DEFAULT_FG_COLOR,
                              DEFAULT_BG_COLOR);
            }
        } else {
            buffer[cursor_y][cursor_x] = {.fg_color = DEFAULT_FG_COLOR, .bg_color = DEFAULT_BG_COLOR, .character = c};
            fb::draw_char(c, cursor_x * CELL_SIZE, cursor_y * CELL_SIZE, SCALE, DEFAULT_FG_COLOR,
                          DEFAULT_BG_COLOR);
            cursor_x++;
            if (cursor_x >= cols) {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= rows) {
                    scroll();
                    cursor_y = rows - 1;
                }
            }
        }
    }

    void write(const char* str, const size_t len) {
        if (!str) return;

        for (size_t i = 0; i < len; ++i) {
            putchar(str[i]);
        }

        fb::flush();
    }

    void clear() {
        fb::clear();
        cursor_x = 0;
        cursor_y = 0;
    }
}
