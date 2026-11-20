#include "spells.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.h"

struct longestCombo findLongestMatchingSpell(struct Player player) {
    enum inputs *inputBuffer = player.inputBuffer;

    struct longestCombo result = {0};

    for(size_t i = 0; i < NUM_SPELLS; i++) {
        struct Spell spell = SPELLS[i];
        int comboLength = spell.comboLength;
        int matchCount = 0;

        for(int j = 0; j < comboLength && j < player.inputCount; j++) {
            if(inputBuffer[7 - player.inputCount + j] == spell.combo[j]) {
                matchCount++;
            } else {
                break;
            }
            
        }

        if(matchCount == player.inputCount) {
            result.spell = spell;
            result.length = matchCount;
        }
    }

    
    return result;

}

void castSpell(struct Player *player, struct Spell spell) {
    if(player->mana >= spell.manaCost) {
        
        if(spell.type == ATTACK) {
            player->mana -= spell.manaCost;
            // send spell through UART to opponent
        } else if(spell.type == SHIELD) {
            if(player->shieldActive) {
                // Shield is already active, do nothing
            } else {
                player->shieldActive = true;
                player->shieldHealth = spell.shieldHealth;
                player->mana -= spell.manaCost;
            }
        } else if(spell.type == HEAL) {
            player->health += spell.healthEffect;
            if(player->health > 100) {
                player->health = 100;
            }

        }
    } else {
        // Not enough mana to cast the spell
    }
}

void receiveSpell(struct Player *player, struct Spell spell) {
    if(spell.type == ATTACK) {
        if(player->shieldActive) {
            int damage = abs(spell.healthEffect);
            int shieldDamage = damage < player->shieldHealth ? damage : player->shieldHealth;
            player->shieldHealth -= shieldDamage;
            if(player->shieldHealth <= 0) {
                player->shieldActive = false;
                player->shieldHealth = 0;
                player->health -= (damage - shieldDamage);
            }
        } else {
            player->health += spell.healthEffect;
        }
    } 

    if(player->health <= 0) {
        player->health = 0;
    }
    if(player->health > 100) {
        player->health = 100;
    }
}

void registerInput(struct Player *player, enum inputs input) {
    player->inputCount = player->inputCount < 7 ? player->inputCount + 1 : 7;
    for(int i = 0; i < 6; i++) {
        player->inputBuffer[i] = player->inputBuffer[i + 1];
    }
    player->inputBuffer[6] = input;
    struct longestCombo combo = findLongestMatchingSpell(*player);
    if(combo.length > 0) {
        if(combo.spell.comboLength == combo.length) {
            player->inputCount = 0;
            castSpell(player, combo.spell);
        } else {
            player->inputCount = combo.length;
        }
        
    } else {
        player->inputCount = 1;
    }
}

void registerReset(struct Player *player, volatile bool reset) {
    player->reset = reset;
}

