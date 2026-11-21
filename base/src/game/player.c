#include "spells.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.h"

cast_result_t castSpell(struct Player *player, struct Spell spell) {
    if(player->mana < spell.manaCost) {
        return CAST_NO_MANA;
    }
    if(spell.type == SHIELD && player->shieldActive) {
        return CAST_SHIELD_UP;
    }
    player->mana -= spell.manaCost;

    if(spell.type == SHIELD) {
        player->shieldActive = true;
        player->shieldHealth = spell.shieldHealth;
        player->maxShieldHealth = spell.shieldHealth;
    } else if(spell.type == HEAL) {
        player->health += spell.healthEffect;
        if(player->health > MAX_HEALTH) {
            player->health = MAX_HEALTH;
        }
    }
    // ATTACK: the caller applies it to the opponent (see castShape)
    return CAST_OK;
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

cast_result_t castShape(struct Player *player, struct Player *opponent, bns_shape_t shape,
                        uint32_t now_ms) {
    const struct Spell *spell = spellForShape(shape);
    if(spell == NULL) {
        return CAST_NO_SPELL;
    }
    const cast_result_t result = castSpell(player, *spell);
    if(result == CAST_OK && spell->type == ATTACK) {
        receiveSpell(opponent, *spell);
    }
    player->lastCast = spell;
    player->lastCastMs = now_ms;
    player->lastCastFailed = result != CAST_OK;
    return result;
}

void registerReset(struct Player *player, volatile bool reset) {
    player->reset = reset;
}
