#include <cstdint>
#include <cstring>
#include <font8x8.hpp>

#include "unistd.h"
#include "../../../libc/include/cstring.hpp"
#include "sys/mman.h"

constexpr uint32_t width = 1280;
constexpr uint32_t height = 720;
constexpr uint32_t pitch = 5120;
constexpr uint32_t bpp = 32;
constexpr uint32_t screen_size = (width * height * bpp) / 8;
uint8_t* fbp;
uint8_t screen_buffer[screen_size];

static void put_pixel(const uint32_t x, const uint32_t y, const uint32_t color) {
    const uint64_t offset = static_cast<uint64_t>(pitch) * y + static_cast<uint64_t>(x) * (
        bpp / 8);
    if (offset + sizeof(uint32_t) > screen_size) return;
    auto* location = reinterpret_cast<uint32_t*>(screen_buffer + offset);
    *location = color;
}

static void draw_char(
    const unsigned char c,
    const uint32_t x,
    const uint32_t y,
    const float size
) {
    const unsigned char* glyph = font8x8_basic[c];
    for (uint32_t cy = 0; cy < 8; cy++) {
        for (uint32_t cx = 0; cx < 8; cx++) {
            const uint8_t mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};
            if (!(glyph[cy] & mask[cx])) continue;
            // Calculate rectangle bounds for this pixel using float size
            const uint32_t x0 = x + static_cast<uint32_t>(cx * size);
            const uint32_t y0 = y + static_cast<uint32_t>(cy * size);
            const uint32_t x1 = x + static_cast<uint32_t>((cx + 1) * size);
            const uint32_t y1 = y + static_cast<uint32_t>((cy + 1) * size);
            for (uint32_t py = y0; py < y1; ++py) {
                for (uint32_t px = x0; px < x1; ++px) {
                    put_pixel(px, py, 0xFFFFFF);
                }
            }
        }
    }
}

int main() {
    fbp = static_cast<uint8_t*>(
        mmap(nullptr, screen_size, PROT_READ | PROT_WRITE, MAP_ANON, 3, 0)
    );

    memset(fbp, 0, screen_size);

    char content[512] = {};
    uint32_t len = 0;

    constexpr char prompt[] = "> ";

    while (true) {
        char buf[1024];
        const size_t bytes = read(0, buf, sizeof(buf));
        if (bytes > 0) {
            switch (buf[0]) {
                case '\0':
                    break;
                case '\b':
                    if (len != 0) {
                        content[len - 1] = '\0';
                        len--;
                    }
                    break;
                default:
                    content[len] = *buf;

                    len++;
                    break;
            }
        }

        memset(screen_buffer, 0, screen_size);
        uint32_t x = 0;
        uint32_t y = 0;
        for (size_t i = 0; i < len; i++) {
            if (content[i] == '\0') break;
            if (content[i] == '\r') continue;
            if (content[i] == '\n') {
                x = 0;
                y += 16;
                continue;
            }
            draw_char(content[i], x, y, 2);
            x += 16;
            if (x >= width) {
                x = 0;
                y += 16;
            }
        }

        // memcpy_fast(fbp, screen_buffer, screen_size);
    }

    return 0;
}
