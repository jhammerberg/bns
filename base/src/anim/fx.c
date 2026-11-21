#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // drawn every frame
#endif

#include "fx.h"

uint32_t fx_hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1U ^ (b + 0x7F4A7C15U + (a << 6) + (a >> 2));
    h ^= h >> 15;
    h *= 0x2C1B3C6DU;
    h ^= h >> 12;
    return h;
}

void fx_burst(int32_t x, int32_t y, uint32_t elapsed, uint32_t life_ms, uint32_t seed,
              uint32_t particles, const hub75_color_t *colors, uint32_t count) {
    if (elapsed >= life_ms || count == 0) {
        return;
    }
    const int32_t t = (int32_t)elapsed;
    // fade: earlier colors while young, later ones as it ages
    const hub75_color_t color = colors[elapsed * count / life_ms];
    for (uint32_t i = 0; i < particles; ++i) {
        const uint32_t h = fx_hash(seed, i);
        // velocity in pixels per second: -40..+40 sideways, -60..-10 upwards
        const int32_t vx = (int32_t)(h & 0x7F) * 80 / 127 - 40;
        const int32_t vy = -10 - (int32_t)((h >> 8) & 0x7F) * 50 / 127;
        const int32_t gravity = 90; // pixels per second^2
        const int32_t px = x + vx * t / 1000;
        const int32_t py = y + vy * t / 1000 + gravity * t * t / 2000000;
        hub75_set_pixel(px, py, color);
    }
}

void fx_bolt(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t seed,
             hub75_color_t core, hub75_color_t glow) {
    if (y1 <= y0) {
        return;
    }
    int32_t x = x0;
    for (int32_t y = y0; y <= y1; ++y) {
        // drift towards the target, with a random kink every few rows
        const int32_t target = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        const uint32_t h = fx_hash(seed, (uint32_t)y / 3U);
        x = target + (int32_t)(h % 5U) - 2;
        hub75_set_pixel(x - 1, y, glow);
        hub75_set_pixel(x + 1, y, glow);
        hub75_set_pixel(x, y, core);
    }
}
