// Laptop stand-in for base/src/input.c: keys act as the two wands.
//   1 2 3 4  player 1 draws  triangle zigzag circle V
//   7 8 9 0  player 2 draws  triangle zigzag circle V
// With --serial, shapes also come from a real receiver XIAO's USB log
// (sim_input_receiver_line).

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "input.h"
#include "sim.h"

#define QUEUE_LEN 32

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static bns_packet_t queue[QUEUE_LEN];
static unsigned head, tail; // pop at head, push at tail
static uint8_t seq[3];

static void push(uint8_t wand, bns_shape_t shape) {
    pthread_mutex_lock(&lock);
    if (tail - head < QUEUE_LEN) {
        queue[tail++ % QUEUE_LEN] = (bns_packet_t){
            .version = BNS_PROTOCOL_VERSION,
            .wand_id = wand,
            .seq = seq[wand]++,
            .shape = (uint8_t)shape,
        };
    }
    pthread_mutex_unlock(&lock);
}

// esp/receiver logs each shape it forwards as "... wand 1 seq=20 shape=triangle"
bool sim_input_receiver_line(const char *line) {
    static const char *const names[BNS_SHAPE_COUNT] = {"none", "triangle", "zigzag", "circle", "v"};
    const char *w = strstr(line, "wand ");
    const char *sh = strstr(line, "shape=");
    unsigned wand;
    if (!w || !sh || sscanf(w, "wand %u", &wand) != 1 || wand < 1 || wand > 2) {
        return false;
    }
    sh += strlen("shape=");
    for (int i = 1; i < BNS_SHAPE_COUNT; ++i) {
        const size_t n = strlen(names[i]);
        if (strncmp(sh, names[i], n) == 0 && !isalpha((unsigned char)sh[n])) {
            push((uint8_t)wand, (bns_shape_t)i);
            return true;
        }
    }
    return false;
}

bool sim_input_key(int key) {
    static const char keys[] = "12347890";
    static const bns_shape_t shapes[] = {BNS_SHAPE_TRIANGLE, BNS_SHAPE_ZIGZAG,
                                         BNS_SHAPE_CIRCLE, BNS_SHAPE_V};
    for (int i = 0; keys[i]; ++i) {
        if (key != keys[i]) {
            continue;
        }
        push(i < 4 ? 1 : 2, shapes[i % 4]);
        return true;
    }
    return false;
}

void input_init(void) {} // keyboard / --serial need no setup

bool input_poll(bns_packet_t *out) {
    pthread_mutex_lock(&lock);
    const bool have = head != tail;
    if (have) {
        *out = queue[head++ % QUEUE_LEN];
    }
    pthread_mutex_unlock(&lock);
    return have;
}
