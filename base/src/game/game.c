#include "game.h"
#include "hub75.h"
#include "hud.h"

static void reset_player(struct Player *p) {
    *p = (struct Player){
        .health = MAX_HEALTH,
        .mana = MAX_MANA,
        .lastCast = NULL,
    };
}

static void enter(game_t *game, game_phase_t phase, uint32_t now_ms) {
    game->phase = phase;
    game->phase_start_ms = now_ms;
    game->last_regen_ms = now_ms;
}

void game_init(game_t *game, uint32_t now_ms) {
    reset_player(&game->players[0]);
    reset_player(&game->players[1]);
    enter(game, GAME_COUNTDOWN ? PHASE_COUNTDOWN : PHASE_PLAYING, now_ms);
}

void game_cast(game_t *game, uint8_t wand_id, bns_shape_t shape, uint32_t now_ms) {
    if (wand_id < 1 || wand_id > 2) {
        return;
    }
    switch (game->phase) {
    case PHASE_COUNTDOWN:
        break; // too early
    case PHASE_PLAYING: {
        struct Player *me = &game->players[wand_id - 1];
        struct Player *opponent = &game->players[2 - wand_id];
        castShape(me, opponent, shape, now_ms);
        break;
    }
    case PHASE_OVER:
        if (now_ms - game->phase_start_ms >= RESTART_LOCKOUT_MS) {
            game_init(game, now_ms);
        }
        break;
    }
}

static void regen_mana(struct Player *p) {
    p->mana += MANA_REGEN_RATE;
    if (p->mana > MAX_MANA) {
        p->mana = MAX_MANA;
    }
}

void game_tick(game_t *game, uint32_t now_ms) {
    switch (game->phase) {
    case PHASE_COUNTDOWN:
        if (now_ms - game->phase_start_ms >= COUNTDOWN_MS) {
            enter(game, PHASE_PLAYING, now_ms);
        }
        break;
    case PHASE_PLAYING:
        while (now_ms - game->last_regen_ms >= MANA_REGEN_MS) {
            game->last_regen_ms += MANA_REGEN_MS;
            regen_mana(&game->players[0]);
            regen_mana(&game->players[1]);
        }
        if (game->players[0].health <= 0 || game->players[1].health <= 0) {
            enter(game, PHASE_OVER, now_ms);
        }
        break;
    case PHASE_OVER:
        break; // waits for a cast (game_cast)
    }
}

// One panel's view of the game for `me` against `opponent`.
static void draw_view(const game_t *game, const struct Player *me, const struct Player *opponent,
                      uint32_t now_ms) {
    hub75_clear();
    switch (game->phase) {
    case PHASE_COUNTDOWN: {
        const uint32_t elapsed = now_ms - game->phase_start_ms;
        draw_countdown_number((char)('3' - (char)(elapsed / 1000U)));
        break;
    }
    case PHASE_PLAYING:
        draw_game(me, opponent, now_ms);
        break;
    case PHASE_OVER:
        draw_victory_screen(me->health > 0);
        break;
    }
}

// Each player sees their own panel: player 1 on panel 0 (wired to the XMC),
// player 2 on panel 1. With a single panel only player 1's view is drawn.
void game_draw(const game_t *game, uint32_t now_ms) {
    hub75_select_panel(0);
    draw_view(game, &game->players[0], &game->players[1], now_ms);
    if (HUB75_PANELS > 1) {
        hub75_select_panel(1);
        draw_view(game, &game->players[1], &game->players[0], now_ms);
        hub75_select_panel(0);
    }
}
