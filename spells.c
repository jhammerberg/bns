#define MAX_COMBO 7
#include "spells.h"

enum inputs {
    up, // 0
    right, // 1
    down, // 2
    left, // 3
    none // 4
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

static const struct Spell SPELLS[] = {
    { // FIREBALL
        .spellName = "FIREBALL",
        .type = ATTACK,
        .manaCost = 10,
        .healthEffect = -10,
        .shieldHealth = 0,
        .comboLength = 3,
        .combo = {up, right, left}
    },
    { // LIGHTNING
        .spellName = "LIGHTNING",
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
        .spellName = "FORTRESS",
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

#define NUM_SPELLS (sizeof(SPELLS) / sizeof(SPELLS[0]))
