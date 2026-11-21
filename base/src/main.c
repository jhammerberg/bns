// BNS base station: XMC4700 driving two chained HUB75 panels, with wand
// shapes arriving from the receiver XIAO over UART.
//
// The display refreshes itself from a timer interrupt (display/hub75.c); this
// loop only runs the game: read input, update, draw, present, once per frame.

#include <stdio.h>
#include "cybsp.h"
#include "game.h"
#include "hub75.h"
#include "input.h"
#include "stats.h"
#include "timer.h"

// printf -> debug UART (J-Link virtual COM port on X101)
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            XMC_UART_CH_Transmit(CYBSP_DEBUG_UART_HW, '\r');
        }
        XMC_UART_CH_Transmit(CYBSP_DEBUG_UART_HW, ptr[i]);
    }
    return len;
}

int main(void) {
    if (cybsp_init() != CY_RSLT_SUCCESS) {
        for (;;) {
        }
    }
    SystemCoreClockUpdate();
    setvbuf(stdout, NULL, _IONBF, 0); // unbuffered: no malloc'd buffer, lines go out at once
    timer_init();
    input_init(); // receiver XIAO on UART (Arduino D0/D1)
    if (hub75_init(SystemCoreClock) != HUB75_OK) {
        for (;;) {
        } // initialization errors leave OE high (blank)
    }
    printf("bns base up\n");

    static game_t game;
    game_init(&game, millis());

    for (;;) {
        const uint32_t start_us = micros();
        const uint32_t now = millis();

        bns_packet_t pkt;
        while (input_poll(&pkt)) {
            game_cast(&game, pkt.wand_id, (bns_shape_t)pkt.shape, now);
        }
        game_tick(&game, now);
        game_draw(&game, now);

        stats_game_time(micros() - start_us);
        hub75_present(); // waits for the next frame boundary
        stats_report(millis());
    }
}
