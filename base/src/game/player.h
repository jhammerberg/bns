#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include "spells.h"

#define MANA_REGEN_RATE 5
#define MAX_HEALTH 100
#define MAX_MANA 100

typedef enum {
    CAST_OK,
    CAST_NO_SPELL,  // shape has no spell
    CAST_NO_MANA,   // not enough mana
    CAST_SHIELD_UP, // shield already active
} cast_result_t;

struct Player {
    int health;
    int mana;
    bool shieldActive;
    int shieldHealth;

    // last spell attempt (NULL: none yet); a fizzle shows a red symbol
    const struct Spell *lastCast;
    uint32_t lastCastMs;  // millis() when it was cast
    bool lastCastFailed;  // true: shape recognised but the spell fizzled

    uint32_t hitMs;       // when damage last landed (castle flashes)

    // health bar animation: glides from shownFrom to shownTo starting at shownMs
    int shownFrom, shownTo;
    uint32_t shownMs;
};

#define HEALTH_GLIDE_MS 400

cast_result_t castSpell(struct Player *player, struct Spell spell);
void receiveSpell(struct Player *player, struct Spell spell);
// A wand drew `shape` at time now_ms: cast the matching spell if affordable.
// Shields and heals take effect now; attacks are returned in *spell_out for
// the game to launch (they land later, see game.c). Records the attempt in
// player->lastCast* for the HUD.
cast_result_t castShape(struct Player *player, bns_shape_t shape, uint32_t now_ms,
                        const struct Spell **spell_out);

// the health value the bar shows at `now_ms` (glides after every change)
int player_shown_health(const struct Player *player, uint32_t now_ms);

// call once per tick: starts a glide when health has changed
void player_track_health(struct Player *player, uint32_t now_ms);

#endif // PLAYER_H
