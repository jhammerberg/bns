#include "stats.h"
#include <stdio.h>
#include "hub75.h"

static uint32_t game_sum, game_max, game_count;
static uint32_t rx_bytes, packets_ok, packets_bad;
static uint32_t last_report_ms, last_frames, last_busy_us;

void stats_game_time(uint32_t us) {
    if (us > 1000000U) {
        return; // micros() wrapped during the measurement
    }
    game_sum += us;
    game_count++;
    if (us > game_max) {
        game_max = us;
    }
}

void stats_rx_bytes(uint32_t count) {
    rx_bytes += count;
}

void stats_packet(bool ok) {
    if (ok) {
        packets_ok++;
    } else {
        packets_bad++;
    }
}

void stats_report(uint32_t now_ms) {
    const uint32_t elapsed = now_ms - last_report_ms;
    if (elapsed < 1000U) {
        return;
    }
    uint32_t frames, busy_us;
    hub75_get_stats(&frames, &busy_us);

    printf("fps %lu | display %lu%% cpu | game avg %lu us max %lu us | rx %lu B %lu ok %lu bad\n",
           (unsigned long)((frames - last_frames) * 1000U / elapsed),
           (unsigned long)((busy_us - last_busy_us) / (elapsed * 10U)),
           (unsigned long)(game_count ? game_sum / game_count : 0),
           (unsigned long)game_max,
           (unsigned long)rx_bytes, (unsigned long)packets_ok, (unsigned long)packets_bad);

    last_report_ms = now_ms;
    last_frames = frames;
    last_busy_us = busy_us;
    game_sum = game_max = game_count = 0;
    rx_bytes = packets_ok = packets_bad = 0;
}
