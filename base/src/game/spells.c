#include "spells.h"

const struct Spell SPELLS[] = {
    {
        .spellName = "FIRE",
        .type = ATTACK,
        .manaCost = 10,
        .healthEffect = -10,
        .shieldHealth = 0,
        .shape = BNS_SHAPE_TRIANGLE,
        .symbol = &SPRITE_FIRE
    },
    {
        .spellName = "SHOCK",
        .type = ATTACK,
        .manaCost = 20,
        .healthEffect = -25,
        .shieldHealth = 0,
        .shape = BNS_SHAPE_ZIGZAG,
        .symbol = &SPRITE_LIGHTNING
    },
    {
        .spellName = "SHIELD",
        .type = SHIELD,
        .manaCost = 15,
        .healthEffect = 0,
        .shieldHealth = 10,
        .shape = BNS_SHAPE_CIRCLE,
        .symbol = &SPRITE_SHIELD
    },
    {
        .spellName = "HEAL",
        .type = HEAL,
        .manaCost = 20,
        .healthEffect = 20,
        .shieldHealth = 0,
        .shape = BNS_SHAPE_V,
        .symbol = &SPRITE_HEAL
    }
};

const size_t NUM_SPELLS = sizeof(SPELLS) / sizeof(SPELLS[0]);

const struct Spell *spellForShape(bns_shape_t shape) {
    for (size_t i = 0; i < NUM_SPELLS; i++) {
        if (SPELLS[i].shape == shape) {
            return &SPELLS[i];
        }
    }
    return NULL;
}
