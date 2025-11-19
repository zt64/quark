#pragma once

#include "boot/limine/limine.h"

struct Framebuffer {
    void* addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint64_t size;
};

namespace screen {
    inline Framebuffer framebuffer;

    void fb_init(const limine_framebuffer* fb);

    void draw_char(unsigned char c, uint32_t x, uint32_t y, float size, uint32_t fgcolor, uint32_t bgcolor);

    void draw(const char* str, uint32_t x, uint32_t y, float size);

    void draw(const char* str, uint32_t x, uint32_t y, float size, uint32_t fgcolor);

    // void draw_rect(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height);
    //
    // void draw_rect(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, uint32_t color);
    //
    // void draw_rect_outline(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, uint32_t border_width);
    //
    // void draw_rect_outline(
    //     uint32_t start_x,
    //     uint32_t start_y,
    //     uint32_t width,
    //     uint32_t height,
    //     uint32_t stroke_width,
    //     uint32_t color
    // );

    void put_pixel(uint32_t x, uint32_t y, uint32_t color);

    void clear();

    /**
     * Flush the buffer to the screen
     */
    void flush();
}
