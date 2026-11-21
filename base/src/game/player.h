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
    int id;
    int health;
    int mana;
    bool shieldActive;
    int shieldHealth;
    int maxShieldHealth;
    volatile bool reset; // volatile: set from the reset button interrupt

    // last spell attempt, for the HUD's cast card (NULL: none yet)
    const struct Spell *lastCast;
    uint32_t lastCastMs;  // millis() when it was cast
    bool lastCastFailed;  // true: shape recognised but the spell fizzled
};

cast_result_t castSpell(struct Player *player, struct Spell spell);
void receiveSpell(struct Player *player, struct Spell spell);
// A wand drew `shape` at time now_ms: cast the matching spell if affordable;
// attacks hit `opponent`. Records the attempt in player->lastCast* for the HUD.
cast_result_t castShape(struct Player *player, struct Player *opponent, bns_shape_t shape,
                        uint32_t now_ms);

void registerReset(struct Player *player, bool reset);

#endif // PLAYER_H
