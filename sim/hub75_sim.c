// Laptop stand-in for base/src/display/hub75.c (the XMC scan-out code), plus
// the BSP stubs from stubs/cybsp.h. Drawing is the real hub75_fb.c.
//
// Like the real panels, the virtual panels only change when the firmware calls
// hub75_refresh(), and go dark if it stops calling it.

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cybsp.h"
#include "hub75_fb.h"
#include "sim.h"

// The real panels stay lit only while hub75_refresh() keeps scanning; after
// this long without a refresh the sim shows them dark.
#define DARK_AFTER_US 100000u

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t glass[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];
static uint64_t last_refresh_us;
static bool blanked = true;
static bool initialized;
static uint32_t refresh_count, refreshes_per_s;
static uint64_t rate_window_us;

uint64_t sim_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static void sleep_us(uint64_t us) {
    struct timespec ts = {(time_t)(us / 1000000u), (long)(us % 1000000u) * 1000L};
    nanosleep(&ts, NULL);
}

// ---- hub75.h hardware functions ----

hub75_status_t hub75_init(uint32_t cpu_hz) {
    initialized = false;
    hub75_fb_reset();
    if (cpu_hz < UINT32_C(1000000) || cpu_hz > UINT32_C(144000000)) {
        return HUB75_ERROR_CLOCK; // same check as the real driver
    }
    initialized = true;
    return HUB75_OK;
}

void hub75_refresh(void) {
    if (!initialized) {
        return;
    }
    pthread_mutex_lock(&lock);
    memcpy(glass, hub75_framebuffer, sizeof glass);
    const uint64_t now = sim_now_us();
    last_refresh_us = now;
    blanked = false;
    refresh_count++;
    if (rate_window_us == 0) {
        rate_window_us = now; // first refresh starts the 1 s counting window
    } else if (now - rate_window_us >= 1000000u) {
        refreshes_per_s = refresh_count;
        refresh_count = 0;
        rate_window_us = now;
    }
    pthread_mutex_unlock(&lock);

    // Take about as long as the real busy-wait scan (hub75.c): per row pair,
    // 2 us per column across the chain plus ~25 us latch/address/on-time.
    sleep_us((HUB75_HEIGHT / 2u) * (HUB75_CHAIN_WIDTH * 2u + 25u));
}

void hub75_blank(void) {
    if (!initialized) {
        return;
    }
    pthread_mutex_lock(&lock);
    blanked = true;
    pthread_mutex_unlock(&lock);
}

void sim_glass_read(sim_glass_t *out) {
    pthread_mutex_lock(&lock);
    memcpy(out->px, glass, sizeof glass);
    out->lit = !blanked && sim_now_us() - last_refresh_us < DARK_AFTER_US;
    out->refreshes_per_s = out->lit ? refreshes_per_s : 0;
    pthread_mutex_unlock(&lock);
}

// ---- BSP / XMCLib stubs ----

uint32_t SystemCoreClock = 144000000u;

cy_rslt_t cybsp_init(void) {
    return CY_RSLT_SUCCESS;
}

void XMC_Delay(uint32_t ms) {
    sleep_us((uint64_t)ms * 1000u);
}

// ---- debug UART -> out/uart.log + tail for the status area ----

static pthread_mutex_t uart_lock = PTHREAD_MUTEX_INITIALIZER;
static char uart_ring[4096];
static size_t uart_len;
static FILE *uart_log;

static void uart_put(char c) {
    if (!uart_log) {
        uart_log = fopen("out/uart.log", "w");
    }
    if (uart_log) {
        fputc(c, uart_log);
        if (c == '\n') fflush(uart_log);
    }
    if (c == '\r') {
        return;
    }
    if (uart_len == sizeof uart_ring) {
        memmove(uart_ring, uart_ring + 1024, sizeof uart_ring - 1024);
        uart_len -= 1024;
    }
    uart_ring[uart_len++] = c;
}

void XMC_UART_CH_Transmit(XMC_USIC_CH_t *channel, uint16_t data) {
    (void)channel;
    pthread_mutex_lock(&uart_lock);
    uart_put((char)data);
    pthread_mutex_unlock(&uart_lock);
}

#undef printf
int sim_uart_printf(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    pthread_mutex_lock(&uart_lock);
    for (const char *p = buf; *p; ++p) uart_put(*p);
    pthread_mutex_unlock(&uart_lock);
    return n;
}

void sim_uart_tail(char *out, size_t n, int lines) {
    pthread_mutex_lock(&uart_lock);
    size_t start = uart_len;
    int seen = 0;
    while (start > 0) {
        if (uart_ring[start - 1] == '\n' && start != uart_len && ++seen >= lines) break;
        start--;
    }
    size_t len = uart_len - start < n - 1 ? uart_len - start : n - 1;
    memcpy(out, uart_ring + start, len);
    out[len] = '\0';
    pthread_mutex_unlock(&uart_lock);
}
