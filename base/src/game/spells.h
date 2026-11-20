#ifndef SPELLS_H
#define SPELLS_H

#include <stddef.h>

#define MAX_COMBO 7

enum inputs {
    up,    // 0
    right, // 1
    down,  // 2
    left,  // 3
    none   // 4
};

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
    int comboLength;
    enum inputs combo[MAX_COMBO];
} Spell;

// Defined in spells.c. Both boards use the same table, so a spell can be
// sent over UART as its index into SPELLS.
extern const struct Spell SPELLS[];
extern const size_t NUM_SPELLS;

#endif // SPELLS_H
