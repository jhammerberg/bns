#include "spells.h"
#include <stdbool.h>
#include <stdlib.h>
#include "player.h"
#include "tween.h"

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

cast_result_t castShape(struct Player *player, bns_shape_t shape, uint32_t now_ms,
                        const struct Spell **spell_out) {
    const struct Spell *spell = spellForShape(shape);
    *spell_out = spell;
    if(spell == NULL) {
        return CAST_NO_SPELL;
    }
    const cast_result_t result = castSpell(player, *spell);
    player->lastCast = spell;
    player->lastCastMs = now_ms;
    player->lastCastFailed = result != CAST_OK;
    return result;
}

int player_shown_health(const struct Player *player, uint32_t now_ms) {
    const int32_t t = tween_ease_out(tween_progress(now_ms - player->shownMs, HEALTH_GLIDE_MS));
    return (int)tween_lerp(player->shownFrom, player->shownTo, t);
}

void player_track_health(struct Player *player, uint32_t now_ms) {
    if(player->health != player->shownTo) {
        player->shownFrom = player_shown_health(player, now_ms); // glide on from here
        player->shownTo = player->health;
        player->shownMs = now_ms;
    }
}

