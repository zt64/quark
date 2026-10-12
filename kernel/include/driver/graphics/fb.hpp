#pragma once

#include "boot/limine/limine.h"

namespace fb {
    struct Framebuffer {
        void* addr;
        uint32_t width;
        uint32_t height;
        uint32_t pitch;
        uint8_t bpp;
        uint64_t size;
    };

    inline Framebuffer framebuffer;

    inline bool initialized;

    void init(const limine_framebuffer* fb);

    void draw_char(unsigned char c, uint32_t x, uint32_t y, float size, uint32_t fgcolor, uint32_t bgcolor);

    void draw(const char* str, uint32_t x, uint32_t y, float size);

    void draw(const char* str, uint32_t x, uint32_t y, float size, uint32_t fgcolor);

    /**
     * Put a pixel on the framebuffer
     * @param x The x coordinate of the pixel
     * @param y The y coordinate of the pixel
     * @param color The color of the pixel
     */
    void put_pixel(uint32_t x, uint32_t y, uint32_t color);

    /**
     * Clear the framebuffer
     */
    void clear();

    /**
     * Flush the buffer to the fb
     */
    void flush();
}
