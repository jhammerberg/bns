// Laptop tests for base/src/anim (clip timing and events):  make test
#include <stdio.h>
#include "anim.h"

static int failures;
#define EXPECT(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static const uint8_t px[1] = {1};
static const uint8_t *const frames[3] = {px, px, px};
static const uint16_t ms[3] = {100, 200, 300};
static const uint8_t events[3] = {5, 0, 7};

int main(void) {
    const anim_clip_t loop = {"loop", ANIM_LOOP, 3, frames, ms, events};
    const anim_clip_t once = {"once", ANIM_ONCE, 3, frames, ms, events};
    const anim_clip_t ping = {"ping", ANIM_PINGPONG, 3, frames, ms, events};

    // loop: 0 for 100 ms, 1 for 200, 2 for 300, then again
    EXPECT(anim_length(&loop) == 600);
    EXPECT(anim_frame_at(&loop, 0) == 0);
    EXPECT(anim_frame_at(&loop, 99) == 0);
    EXPECT(anim_frame_at(&loop, 100) == 1);
    EXPECT(anim_frame_at(&loop, 299) == 1);
    EXPECT(anim_frame_at(&loop, 300) == 2);
    EXPECT(anim_frame_at(&loop, 600) == 0);
    EXPECT(!anim_finished(&loop, 10000));

    // once: holds the last frame and reports finished
    EXPECT(anim_frame_at(&once, 5000) == 2);
    EXPECT(!anim_finished(&once, 599));
    EXPECT(anim_finished(&once, 600));

    // ping-pong: 0 1 2 1 0 ... (the way back skips both ends)
    EXPECT(anim_length(&ping) == 800);
    EXPECT(anim_frame_at(&ping, 650) == 1);
    EXPECT(anim_frame_at(&ping, 800) == 0);

    // events: frame 0 carries 5 (at 0, 600, ...), frame 2 carries 7 (at 300, 900, ...)
    EXPECT(anim_event_between(&loop, 0, 1, 5));   // fires on the very first check
    EXPECT(!anim_event_between(&loop, 1, 300, 5));
    EXPECT(anim_event_between(&loop, 250, 350, 7));
    EXPECT(anim_event_between(&loop, 590, 610, 5)); // second pass
    EXPECT(!anim_event_between(&once, 590, 2000, 5)); // once: no second pass
    EXPECT(!anim_event_between(&loop, 300, 300, 7)); // empty window

    printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
