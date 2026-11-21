// Sprite animation: clips of palette-indexed frames, played as a pure
// function of time. Art lives in art/*.sprite, compiled by tools/gen_sprites.py
// into assets/anim_assets.c.
//
// A player is just (clip, start time): the frame showing at `now` is computed,
// nothing is stepped per frame, so animations can't drift and are easy to test.

#ifndef ANIM_H
#define ANIM_H

#include <stdbool.h>
#include <stdint.h>
#include "hub75.h"

typedef enum {
    ANIM_LOOP,     // 0 1 2 0 1 2 ...
    ANIM_ONCE,     // 0 1 2 2 2 ... (holds the last frame; anim_finished() turns true)
    ANIM_PINGPONG, // 0 1 2 1 0 1 2 ...
} anim_mode_t;

typedef struct {
    const char *name;
    uint8_t mode;              // anim_mode_t
    uint8_t count;             // frames
    const uint8_t *const *frames; // palette indices, row by row; 0 = transparent
    const uint16_t *ms;        // duration of each frame
    const uint8_t *events;     // ANIM_EVENT_* fired when each frame starts; 0 = none
} anim_clip_t;

typedef struct {
    const char *name;
    uint8_t width, height;
    const hub75_color_t *palette; // 16 entries; [0] unused (transparent)
    uint8_t clip_count;
    const anim_clip_t *clips;
} anim_sprite_t;

// total length of one pass through the clip (ping-pong: there and back)
uint32_t anim_length(const anim_clip_t *clip);

// frame showing `elapsed` ms after the clip started
uint32_t anim_frame_at(const anim_clip_t *clip, uint32_t elapsed);

// true once a ANIM_ONCE clip has shown its last frame for its full duration
bool anim_finished(const anim_clip_t *clip, uint32_t elapsed);

// true if a frame carrying `event` starts in [from, to) ms after the clip
// start. Check consecutive windows ([prev, now)) and each event fires once.
bool anim_event_between(const anim_clip_t *clip, uint32_t from, uint32_t to, uint8_t event);

#define ANIM_FLIP_H 1U // mirror left-right
#define ANIM_FLIP_V 2U // mirror top-bottom

// Draw the frame showing at `elapsed`, top-left at (x, y). `palette` replaces
// the sprite's colours if not NULL (palette swaps, hit flashes); `flags` are
// ANIM_FLIP_*.
void anim_draw(const anim_sprite_t *sprite, const anim_clip_t *clip, uint32_t elapsed,
               int32_t x, int32_t y, const hub75_color_t *palette, uint32_t flags);

// same, centred on (cx, cy)
void anim_draw_centered(const anim_sprite_t *sprite, const anim_clip_t *clip, uint32_t elapsed,
                        int32_t cx, int32_t cy, const hub75_color_t *palette, uint32_t flags);

#endif // ANIM_H
