#ifndef SPELLS_H
#define SPELLS_H

#include <stddef.h>
#include "protocol.h"
#include "sprites.h"

enum spellType {
    ATTACK,
    SHIELD,
    HEAL
};

typedef struct Spell {
    char* spellName;
    enum spellType type;
    int manaCost;
    int healthEffect;
    int shieldHealth;
    bns_shape_t shape;      // drawn with the wand to cast this spell
    const sprite_t *symbol; // shown on the HUD next to the shape
} Spell;

// Defined in spells.c, one spell per wand shape.
extern const struct Spell SPELLS[];
extern const size_t NUM_SPELLS;

// The spell cast by drawing `shape`, or NULL if no spell uses it.
const struct Spell *spellForShape(bns_shape_t shape);

#endif // SPELLS_H
