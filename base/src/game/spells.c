#define MAX_COMBO 7
#include "spells.h"
#include "protocol.h"

// Radio packets carry enum inputs values directly; keep them in sync.
_Static_assert((int)BNS_DIR_UP == (int)up && (int)BNS_DIR_RIGHT == (int)right &&
               (int)BNS_DIR_DOWN == (int)down && (int)BNS_DIR_LEFT == (int)left &&
               (int)BNS_DIR_NONE == (int)none,
               "common/protocol.h directions must match enum inputs");
const struct Spell SPELLS[] = {
    { // FIREBALL
        .spellName = "FIRE",
        .type = ATTACK,
        .manaCost = 10,
        .healthEffect = -10,
        .shieldHealth = 0,
        .comboLength = 3,
        .combo = {up, right, left}
    },
    { // LIGHTNING
        .spellName = "SHOCK",
        .type = ATTACK,
        .manaCost = 20,
        .healthEffect = -25,
        .shieldHealth = 0,
        .comboLength = 7,
        .combo = {up, right, down, left, up, right, down}
    },
    { // SHIELD
        .spellName = "SHIELD",
        .type = SHIELD,
        .manaCost = 15,
        .healthEffect = 0,
        .shieldHealth = 10,
        .comboLength = 4,
        .combo = {down, down, up, up}
    },
    { // FORTRESS
        .spellName = "FORT",
        .type = SHIELD,
        .manaCost = 25,
        .healthEffect = 0,
        .shieldHealth = 30,
        .comboLength = 7,
        .combo = {down, down, up, right, down, right, left}
    },
    { // HEAL
        .spellName = "HEAL",
        .type = HEAL,
        .manaCost = 20,
        .healthEffect = 20,
        .shieldHealth = 0,
        .comboLength = 7,
        .combo = {right, up, down, down, up, down, left}
    }
};

const size_t NUM_SPELLS = sizeof(SPELLS) / sizeof(SPELLS[0]);
