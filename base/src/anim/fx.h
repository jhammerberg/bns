// Procedural effects. Like clips, these are pure functions of time: a burst is
// fully described by its origin, start time and seed, so nothing is stored
// per particle and nothing is stepped per frame.

#ifndef FX_H
#define FX_H

#include <stdint.h>
#include "hub75.h"

// Particles flying out from (x, y) and falling, fading through `colors`
// (count entries, brightest first). Lasts `life_ms`; `seed` picks the pattern.
void fx_burst(int32_t x, int32_t y, uint32_t elapsed, uint32_t life_ms, uint32_t seed,
              uint32_t particles, const hub75_color_t *colors, uint32_t count);

// A jagged bolt from (x0, y0) down to (x1, y1); `seed` picks the zigzag.
void fx_bolt(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t seed,
             hub75_color_t core, hub75_color_t glow);

// small deterministic hash (pseudo-random from integers)
uint32_t fx_hash(uint32_t a, uint32_t b);

#endif // FX_H
