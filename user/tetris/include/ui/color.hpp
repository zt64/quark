#pragma once
#include <cstdint>

inline uint32_t clamp8(const int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint32_t>(v);
}

static uint32_t lighten_color(const uint32_t c, const uint8_t pct) {
    const int r = static_cast<int>((c >> 16) & 0xFF);
    const int g = static_cast<int>((c >> 8) & 0xFF);
    const int b = static_cast<int>(c & 0xFF);
    const int nr = r + ((255 - r) * pct) / 100;
    const int ng = g + ((255 - g) * pct) / 100;
    const int nb = b + ((255 - b) * pct) / 100;
    return (clamp8(nr) << 16) | (clamp8(ng) << 8) | clamp8(nb);
}

static uint32_t darken_color(const uint32_t c, const uint8_t pct) {
    const int r = static_cast<int>((c >> 16) & 0xFF);
    const int g = static_cast<int>((c >> 8) & 0xFF);
    const int b = static_cast<int>(c & 0xFF);
    const int nr = (r * (100 - pct)) / 100;
    const int ng = (g * (100 - pct)) / 100;
    const int nb = (b * (100 - pct)) / 100;
    return (clamp8(nr) << 16) | (clamp8(ng) << 8) | clamp8(nb);
}