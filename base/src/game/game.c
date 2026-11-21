#include "game.h"
#include "hub75.h"
#include "hud.h"
#include "scene.h"

static void reset_player(struct Player *p, uint32_t now_ms) {
    *p = (struct Player){
        .health = MAX_HEALTH,
        .mana = MAX_MANA,
        .lastCast = NULL,
        .hitMs = now_ms - 100000U, // long ago: no flash at the start
        .shownFrom = MAX_HEALTH,
        .shownTo = MAX_HEALTH,
        .shownMs = now_ms,
    };
}

uint32_t effect_impact_ms(const effect_t *effect) {
    return effect->kind == EFFECT_FIREBALL ? FIREBALL_FLIGHT_MS : LIGHTNING_STRIKE_MS;
}

uint32_t effect_total_ms(const effect_t *effect) {
    return effect->kind == EFFECT_FIREBALL ? FIREBALL_TOTAL_MS : LIGHTNING_TOTAL_MS;
}

static void land(game_t *game, effect_t *effect, uint32_t now_ms) {
    struct Player *target = &game->players[1 - effect->caster];
    receiveSpell(target, *effect->spell);
    target->hitMs = now_ms;
    effect->landed = true;
}

static void launch(game_t *game, uint8_t caster, const struct Spell *spell, uint32_t now_ms) {
    for (int i = 0; i < MAX_EFFECTS; ++i) {
        effect_t *e = &game->effects[i];
        if (!e->active) {
            *e = (effect_t){
                .active = true,
                .kind = spell->shape == BNS_SHAPE_ZIGZAG ? EFFECT_LIGHTNING : EFFECT_FIREBALL,
                .caster = caster,
                .start_ms = now_ms,
                .spell = spell,
            };
            return;
        }
    }
    // no free slot (only with silly spam): resolve at once
    effect_t instant = {.caster = caster, .spell = spell};
    land(game, &instant, now_ms);
}

static void enter(game_t *game, game_phase_t phase, uint32_t now_ms) {
    game->phase = phase;
    game->phase_start_ms = now_ms;
    game->last_regen_ms = now_ms;
}

void game_init(game_t *game, uint32_t now_ms) {
    reset_player(&game->players[0], now_ms);
    reset_player(&game->players[1], now_ms);
    for (int i = 0; i < MAX_EFFECTS; ++i) {
        game->effects[i].active = false;
    }
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
        const uint8_t caster = (uint8_t)(wand_id - 1);
        const struct Spell *spell;
        if (castShape(&game->players[caster], shape, now_ms, &spell) == CAST_OK &&
            spell->type == ATTACK) {
            launch(game, caster, spell, now_ms);
        }
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
        for (int i = 0; i < MAX_EFFECTS; ++i) {
            effect_t *e = &game->effects[i];
            if (!e->active) {
                continue;
            }
            const uint32_t age = now_ms - e->start_ms;
            if (!e->landed && age >= effect_impact_ms(e)) {
                land(game, e, now_ms);
            }
            if (age >= effect_total_ms(e)) {
                e->active = false;
            }
        }
        player_track_health(&game->players[0], now_ms);
        player_track_health(&game->players[1], now_ms);
        if (game->players[0].health <= 0 || game->players[1].health <= 0) {
            enter(game, PHASE_OVER, now_ms);
        }
        break;
    case PHASE_OVER:
        break; // waits for a cast (game_cast)
    }
}

// One panel's view of the game for `me` against `opponent`.
static void draw_view(const game_t *game, uint32_t viewer, uint32_t now_ms) {
    const struct Player *me = &game->players[viewer];
    hub75_clear();
    switch (game->phase) {
    case PHASE_COUNTDOWN: {
        const uint32_t elapsed = now_ms - game->phase_start_ms;
        draw_countdown_number((char)('3' - (char)(elapsed / 1000U)));
        break;
    }
    case PHASE_PLAYING:
        scene_draw(game, viewer, now_ms);                       // castles + spells
        draw_game(me, &game->players[1U - viewer], now_ms);     // HUD on top
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
    draw_view(game, 0, now_ms);
    if (HUB75_PANELS > 1) {
        hub75_select_panel(1);
        draw_view(game, 1, now_ms);
        hub75_select_panel(0);
    }
}
