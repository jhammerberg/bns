// The game as a state machine. Nothing here blocks or touches hardware:
// main.c calls game_cast() for each wand shape, then game_tick() and
// game_draw() once per frame, then hub75_present().
//
//   COUNTDOWN --3 s--> PLAYING --someone at 0 health--> OVER --cast--> COUNTDOWN

#ifndef GAMES_H
#define GAMES_H

#include <stdint.h>
#include "player.h"

// 3-2-1 countdown before each game. The simulator builds with
// -DGAME_COUNTDOWN=0 so edits show up immediately.
#ifndef GAME_COUNTDOWN
#define GAME_COUNTDOWN 1
#endif

#define COUNTDOWN_MS       3000
#define MANA_REGEN_MS      1000 // +MANA_REGEN_RATE mana this often
#define RESTART_LOCKOUT_MS 2000 // game-over screen ignores casts this long

typedef enum {
    PHASE_COUNTDOWN,
    PHASE_PLAYING,
    PHASE_OVER,
} game_phase_t;

typedef struct {
    game_phase_t phase;
    uint32_t phase_start_ms;
    uint32_t last_regen_ms;
    struct Player players[2]; // [0] = wand 1 = panel 0, [1] = wand 2 = panel 1
} game_t;

// start a new round (countdown first, unless GAME_COUNTDOWN is 0)
void game_init(game_t *game, uint32_t now_ms);

// wand `wand_id` (1 or 2) drew `shape`. During play this casts; on the
// game-over screen it starts the next round.
void game_cast(game_t *game, uint8_t wand_id, bns_shape_t shape, uint32_t now_ms);

// advance timers: countdown, mana regeneration, end of game
void game_tick(game_t *game, uint32_t now_ms);

// draw every panel's view into the back buffer (then call hub75_present)
void game_draw(const game_t *game, uint32_t now_ms);

#endif // GAMES_H
