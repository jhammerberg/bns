// The game as a state machine. Nothing here blocks or touches hardware:
// main.c calls game_cast() for each wand shape, then game_tick() and
// game_draw() once per frame, then hub75_present().
//
//   COUNTDOWN --3 s--> PLAYING --someone at 0 health--> OVER --cast--> COUNTDOWN

#ifndef GAMES_H
#define GAMES_H

#include <stdbool.h>
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

// Attacks in flight. Damage lands at the impact time, not at the cast, so a
// shield raised while a fireball is in the air still blocks it.
typedef enum {
    EFFECT_FIREBALL,  // flies from the caster's castle to the other castle
    EFFECT_LIGHTNING, // a cloud gathers over the target, then strikes
} effect_kind_t;

#define FIREBALL_FLIGHT_MS  700  // cast -> impact
#define FIREBALL_TOTAL_MS   1400 // until the sparks are gone
#define LIGHTNING_STRIKE_MS 450
#define LIGHTNING_TOTAL_MS  1100
#define MAX_EFFECTS         8

typedef struct {
    bool active;
    bool landed;          // damage applied
    uint8_t kind;         // effect_kind_t
    uint8_t caster;       // 0 or 1 (players[] index); the target is the other one
    uint32_t start_ms;
    const struct Spell *spell;
} effect_t;

typedef struct {
    game_phase_t phase;
    uint32_t phase_start_ms;
    uint32_t last_regen_ms;
    struct Player players[2]; // [0] = wand 1 = panel 0, [1] = wand 2 = panel 1
    effect_t effects[MAX_EFFECTS];
} game_t;

// impact and end time of an effect, relative to its start
uint32_t effect_impact_ms(const effect_t *effect);
uint32_t effect_total_ms(const effect_t *effect);

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
