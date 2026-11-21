// Easing for procedural motion and fades. Integer math: t is progress in
// 0..TWEEN_ONE (fixed point), so no floats on the hot path.

#ifndef TWEEN_H
#define TWEEN_H

#include <stdint.h>

#define TWEEN_ONE 1024

// progress of `elapsed` through `duration`, clamped to 0..TWEEN_ONE
static inline int32_t tween_progress(uint32_t elapsed, uint32_t duration) {
    if (duration == 0 || elapsed >= duration) {
        return TWEEN_ONE;
    }
    return (int32_t)(elapsed * TWEEN_ONE / duration);
}

static inline int32_t tween_ease_in(int32_t t) { // slow start: quadratic
    return t * t / TWEEN_ONE;
}

static inline int32_t tween_ease_out(int32_t t) { // slow end
    const int32_t u = TWEEN_ONE - t;
    return TWEEN_ONE - u * u / TWEEN_ONE;
}

static inline int32_t tween_ease_in_out(int32_t t) {
    return t < TWEEN_ONE / 2 ? 2 * t * t / TWEEN_ONE
                             : TWEEN_ONE - 2 * (TWEEN_ONE - t) * (TWEEN_ONE - t) / TWEEN_ONE;
}

// a + (b - a) * t
static inline int32_t tween_lerp(int32_t a, int32_t b, int32_t t) {
    return a + (b - a) * t / TWEEN_ONE;
}

#endif // TWEEN_H
