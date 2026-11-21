// bns-sim: run the real firmware (base/src/main.c) against virtual HUB75
// panels on a laptop.
//
//   ./bns-sim                live view in the terminal until Ctrl-C
//   ./bns-sim --seconds 2    run 2 s, no live view, then exit
//   ./bns-sim --seconds 3 --keys 12.7   scripted key presses, one every
//                            0.5 s ('.' = skip a slot), for headless tests
// Keys 1-4 / 7-0 cast for player 1 / 2 (see input_sim.c); R resets.
//
// Always keeps out/frame.png up to date with what the panels show, and
// writes the firmware's printf output to out/uart.log.

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "sim.h"

#undef printf // sim output goes to the real stdout, not the UART log

int firmware_main(void); // base/src/main.c, renamed in firmware_main.c

#define FRAME_US    33000u  // ~30 fps display update
#define SNAPSHOT_US 250000u // PNG at most 4x per second, only when changed

#define KEY_INTERVAL_US 500000u // --keys: one scripted key per half second

static volatile sig_atomic_t stop;
static struct termios saved_termios;
static bool raw_terminal;

// Read keys one at a time without Enter or echo (Ctrl-C still works).
static void terminal_raw(void) {
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return;
    }
    struct termios t = saved_termios;
    t.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    t.c_cc[VMIN] = 0; // read() returns at once if no key is waiting
    t.c_cc[VTIME] = 0;
    raw_terminal = tcsetattr(STDIN_FILENO, TCSANOW, &t) == 0;
}

static void terminal_restore(void) {
    if (raw_terminal) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        raw_terminal = false;
    }
}

static void on_signal(int sig) {
    (void)sig;
    stop = 1;
}

static void *firmware_thread(void *arg) {
    (void)arg;
    firmware_main();
    fprintf(stderr, "firmware main() returned\n");
    return NULL;
}

int main(int argc, char **argv) {
    double seconds = 0; // 0 = run until Ctrl-C
    const char *script = ""; // --keys
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = atof(argv[++i]);
        } else if (strcmp(argv[i], "--keys") == 0 && i + 1 < argc) {
            script = argv[++i];
        } else {
            fprintf(stderr, "usage: %s [--seconds N] [--keys KEYS]\n", argv[0]);
            return 2;
        }
    }
    const bool live = seconds == 0 && isatty(STDOUT_FILENO);

    mkdir("out", 0755);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    pthread_t fw;
    pthread_create(&fw, NULL, firmware_thread, NULL);

    if (live) {
        fputs("\x1b[?25l\x1b[2J", stdout); // hide cursor, clear screen
        terminal_raw();
    }

    static sim_glass_t g, last;
    bool have_last = false;
    uint64_t last_png = 0;
    const uint64_t start = sim_now_us();

    while (!stop && (seconds == 0 || (double)(sim_now_us() - start) < seconds * 1e6)) {
        const uint64_t now = sim_now_us();

        // keyboard, and scripted keys: script[i] is pressed at (i+1) * 0.5 s
        char key;
        while (raw_terminal && read(STDIN_FILENO, &key, 1) == 1) {
            if (key == 'r' || key == 'R') {
                // like the board's reset button: start the firmware over by
                // re-running this program in place (same args, same pid)
                terminal_restore();
                execv(argv[0], argv);
                perror("reset failed");
            }
            sim_input_key(key);
        }
        static size_t next_key;
        while (script[next_key] && now - start >= (next_key + 1) * KEY_INTERVAL_US) {
            sim_input_key(script[next_key++]);
        }

        sim_glass_read(&g);

        if (live) {
            char status[256], tail[1024];
            snprintf(status, sizeof status,
                     "%ux%u  %s  %u refresh/s   cast: 1-4 player 1, 7-0 player 2 "
                     "(tri zig circ v)   R resets   Ctrl-C quits",
                     (unsigned)HUB75_CHAIN_WIDTH, (unsigned)HUB75_HEIGHT,
                     g.lit ? "LIT" : "DARK (firmware not calling hub75_refresh)",
                     g.refreshes_per_s);
            sim_uart_tail(tail, sizeof tail, 6);
            render_terminal(&g, status, tail);
        }

        const bool changed = !have_last || memcmp(&g.px, &last.px, sizeof g.px) || g.lit != last.lit;
        if (changed && now - last_png >= SNAPSHOT_US) {
            render_png("out/frame.png", &g);
            last = g;
            have_last = true;
            last_png = now;
        }

        struct timespec ts = {0, FRAME_US * 1000L};
        nanosleep(&ts, NULL);
    }

    sim_glass_read(&g);
    render_png("out/frame.png", &g); // final state
    terminal_restore();
    if (live) fputs("\x1b[0m\x1b[?25h\n", stdout); // restore cursor
    printf("panels %s, %u refresh/s; wrote out/frame.png, out/uart.log\n",
           g.lit ? "lit" : "DARK", g.refreshes_per_s);
    return 0; // the firmware thread never returns; exiting ends it
}
