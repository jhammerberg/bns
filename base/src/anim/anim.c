#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // drawn every frame
#endif

#include "anim.h"

uint32_t anim_length(const anim_clip_t *clip) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < clip->count; ++i) {
        total += clip->ms[i];
    }
    if (clip->mode == ANIM_PINGPONG && clip->count > 2) {
        for (uint32_t i = 1; i + 1 < clip->count; ++i) {
            total += clip->ms[i]; // the way back skips both ends
        }
    }
    return total;
}

// frame index for position t within one pass
static uint32_t frame_in_pass(const anim_clip_t *clip, uint32_t t) {
    for (uint32_t i = 0; i < clip->count; ++i) {
        if (t < clip->ms[i]) {
            return i;
        }
        t -= clip->ms[i];
    }
    for (uint32_t i = clip->count - 1; i-- > 1;) { // ping-pong way back
        if (t < clip->ms[i]) {
            return i;
        }
        t -= clip->ms[i];
    }
    return clip->count - 1U;
}

uint32_t anim_frame_at(const anim_clip_t *clip, uint32_t elapsed) {
    const uint32_t length = anim_length(clip);
    if (length == 0) {
        return 0;
    }
    if (clip->mode == ANIM_ONCE) {
        return elapsed >= length ? clip->count - 1U : frame_in_pass(clip, elapsed);
    }
    return frame_in_pass(clip, elapsed % length);
}

bool anim_finished(const anim_clip_t *clip, uint32_t elapsed) {
    return clip->mode == ANIM_ONCE && elapsed >= anim_length(clip);
}

bool anim_event_between(const anim_clip_t *clip, uint32_t from, uint32_t to, uint8_t event) {
    const uint32_t length = anim_length(clip);
    if (to <= from || length == 0) {
        return false;
    }
    // walk frame start times pass by pass until we're past `to`
    for (uint32_t pass_start = 0; pass_start < to; pass_start += length) {
        uint32_t start = pass_start;
        for (uint32_t i = 0; i < clip->count; ++i) {
            if (clip->events[i] == event && start >= from && start < to) {
                return true;
            }
            start += clip->ms[i];
        }
        if (clip->mode == ANIM_PINGPONG) {
            for (uint32_t i = clip->count - 1; i-- > 1;) {
                if (clip->events[i] == event && start >= from && start < to) {
                    return true;
                }
                start += clip->ms[i];
            }
        }
        if (clip->mode == ANIM_ONCE) {
            break;
        }
    }
    return false;
}

void anim_draw(const anim_sprite_t *sprite, const anim_clip_t *clip, uint32_t elapsed,
               int32_t x, int32_t y, const hub75_color_t *palette, uint32_t flags) {
    const uint8_t *pixels = clip->frames[anim_frame_at(clip, elapsed)];
    const hub75_color_t *colors = palette ? palette : sprite->palette;
    const int32_t w = sprite->width, h = sprite->height;
    for (int32_t row = 0; row < h; ++row) {
        const int32_t py = y + ((flags & ANIM_FLIP_V) ? h - 1 - row : row);
        for (int32_t col = 0; col < w; ++col) {
            const uint8_t index = *pixels++;
            if (index) {
                const int32_t px = x + ((flags & ANIM_FLIP_H) ? w - 1 - col : col);
                hub75_set_pixel(px, py, colors[index]);
            }
        }
    }
}

void anim_draw_centered(const anim_sprite_t *sprite, const anim_clip_t *clip, uint32_t elapsed,
                        int32_t cx, int32_t cy, const hub75_color_t *palette, uint32_t flags) {
    anim_draw(sprite, clip, elapsed, cx - sprite->width / 2, cy - sprite->height / 2, palette, flags);
}
