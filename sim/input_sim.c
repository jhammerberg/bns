// Laptop stand-in for base/src/input.c: keys act as the two wands.
//   1 2 3 4  player 1 draws  triangle zigzag circle V
//   7 8 9 0  player 2 draws  triangle zigzag circle V

#include <pthread.h>
#include "input.h"
#include "sim.h"

#define QUEUE_LEN 32

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static bns_packet_t queue[QUEUE_LEN];
static unsigned head, tail; // pop at head, push at tail
static uint8_t seq[3];

bool sim_input_key(int key) {
    static const char keys[] = "12347890";
    static const bns_shape_t shapes[] = {BNS_SHAPE_TRIANGLE, BNS_SHAPE_ZIGZAG,
                                         BNS_SHAPE_CIRCLE, BNS_SHAPE_V};
    for (int i = 0; keys[i]; ++i) {
        if (key != keys[i]) {
            continue;
        }
        const uint8_t wand = i < 4 ? 1 : 2;
        pthread_mutex_lock(&lock);
        if (tail - head < QUEUE_LEN) {
            queue[tail++ % QUEUE_LEN] = (bns_packet_t){
                .version = BNS_PROTOCOL_VERSION,
                .wand_id = wand,
                .seq = seq[wand]++,
                .shape = (uint8_t)shapes[i % 4],
            };
        }
        pthread_mutex_unlock(&lock);
        return true;
    }
    return false;
}

bool input_poll(bns_packet_t *out) {
    pthread_mutex_lock(&lock);
    const bool have = head != tail;
    if (have) {
        *out = queue[head++ % QUEUE_LEN];
    }
    pthread_mutex_unlock(&lock);
    return have;
}
