#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "spells.h"

#define MANA_REGEN_RATE 5
#define MAX_HEALTH 100
#define MAX_MANA 100

struct Player {
    int id;
    int health;
    int mana;
    enum inputs inputBuffer[MAX_COMBO];
    int inputCount;
    bool shieldActive;
    int shieldHealth;
    volatile bool reset; // volatile: set from the reset button interrupt
};

typedef struct longestCombo {
    struct Spell spell;
    int length;
} longestCombo;

struct longestCombo findLongestMatchingSpell(struct Player player);
void castSpell(struct Player *player, struct Spell spell);
void receiveSpell(struct Player *player, struct Spell spell);
void registerInput(struct Player *player, enum inputs input);
void registerReset(struct Player *player, bool reset);

#endif // PLAYER_H
