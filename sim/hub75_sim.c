// Laptop stand-in for base/src/display/hub75.c (the XMC scan-out code), plus
// the BSP stubs from stubs/cybsp.h. Drawing is the real hub75_fb.c.
//
// Like the real display (a timer interrupt), the virtual panels keep showing
// the last presented frame. hub75_present() paces the game loop at the same
// frame rate as the board; if the loop stops presenting, the status line
// says STALLED.

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cybsp.h"
#include "hub75_fb.h"
#include "sim.h"
#include "timer.h"

#define FRAME_US        (1000000u / 120u) // HUB75_FRAME_HZ in hub75.c
#define STALLED_AFTER_US 100000u

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t glass[HUB75_HEIGHT][HUB75_CHAIN_WIDTH]; // panel 0 first
static uint64_t last_present_us, next_frame_us;
static bool blanked;
static bool initialized;
static uint32_t frame_count, frames_in_window, frames_per_s;
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
    last_present_us = next_frame_us = sim_now_us();
    initialized = true;
    return HUB75_OK;
}

void hub75_present(void) {
    if (!initialized) {
        return;
    }
    // wait for the next frame boundary, like the board waits for its interrupt
    const uint64_t now = sim_now_us();
    if (next_frame_us > now) {
        sleep_us(next_frame_us - now);
    }
    next_frame_us = (next_frame_us > now ? next_frame_us : now) + FRAME_US;

    pthread_mutex_lock(&lock);
    hub75_fb_swap();
    // framebuffer columns are in chain (shift) order; the glass is drawn left to
    // right as panel 0, panel 1, ... so reorder each panel into place
    for (uint32_t panel = 0; panel < HUB75_PANELS; ++panel) {
        for (uint32_t y = 0; y < HUB75_HEIGHT; ++y) {
            memcpy(&glass[y][panel * HUB75_WIDTH],
                   &(*hub75_front)[y][HUB75_PANEL_COLUMN(panel)], HUB75_WIDTH);
        }
    }
    const uint64_t t = sim_now_us();
    last_present_us = t;
    frame_count++;
    frames_in_window++;
    if (rate_window_us == 0) {
        rate_window_us = t;
    } else if (t - rate_window_us >= 1000000u) {
        frames_per_s = frames_in_window;
        frames_in_window = 0;
        rate_window_us = t;
    }
    pthread_mutex_unlock(&lock);
}

void hub75_blank(bool blank) {
    pthread_mutex_lock(&lock);
    blanked = blank;
    pthread_mutex_unlock(&lock);
}

void hub75_get_stats(uint32_t *frames, uint32_t *busy_us) {
    pthread_mutex_lock(&lock);
    *frames = frame_count;
    pthread_mutex_unlock(&lock);
    *busy_us = 0; // no display interrupt on a laptop
}

void sim_glass_read(sim_glass_t *out) {
    pthread_mutex_lock(&lock);
    memcpy(out->px, glass, sizeof glass);
    out->lit = initialized && !blanked;
    out->stalled = initialized && sim_now_us() - last_present_us > STALLED_AFTER_US;
    out->frames_per_s = out->stalled ? 0 : frames_per_s;
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

// ---- base/src/timer.h (the real one uses the XMC SysTick) ----

static uint64_t timer_start_us;

void timer_init(void) {
    timer_start_us = sim_now_us();
}

uint32_t millis(void) {
    return (uint32_t)((sim_now_us() - timer_start_us) / 1000u);
}

uint32_t micros(void) {
    return (uint32_t)(sim_now_us() - timer_start_us);
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
