#include "hub75.h"
#include "hub75_text.h"
#include "player.h"
#include "spells.h"
#include "game_assets.h"

void draw_health(int x, int y) {
    hub75_draw_text_small("HEALTH", HEALTH_NAME_X, HEALTH_NAME_Y, HUB75_WHITE, HUB75_BLACK);
}

void draw_mana(int x, int y) {
    hub75_draw_text_small("MANA", MANA_NAME_X, MANA_NAME_Y, HUB75_WHITE, HUB75_BLACK);
}

// 5x4 icons, one byte per row, bit 4 is the left column.
static const uint8_t HEART_ICON[ICON_HEIGHT] = {
    0x1B, // ##.##
    0x1F, // #####
    0x0E, // .###.
    0x04, // ..#..
};

static const uint8_t MANA_ICON[ICON_HEIGHT] = {
    0x04, // ..#..
    0x0E, // .###.
    0x1F, // #####
    0x0E, // .###.
};

static const uint8_t SHIELD_ICON[ICON_HEIGHT] = {
    0x1F, // #####
    0x1F, // #####
    0x1F, // #####
    0x0E, // .###.
};

static void draw_icon(const uint8_t icon[ICON_HEIGHT], int x, int y, hub75_color_t color) {
    for (int row = 0; row < ICON_HEIGHT; row++) {
        for (int col = 0; col < ICON_WIDTH; col++) {
            const bool lit = icon[row] & (1U << (ICON_WIDTH - 1 - col));
            hub75_set_pixel(x + col, y + row, lit ? color : HUB75_BLACK);
        }
    }
}

void draw_health_bar(int totalHealth) {
    draw_icon(HEART_ICON, ICON_X, HEALTH_BAR_Y, HUB75_RED);

    int displayHealth = totalHealth * BAR_WIDTH / MAX_HEALTH;
    if(displayHealth == 0 && totalHealth > 0) {
        displayHealth = 1;
    }

    //background
    hub75_fill_rect(HEALTH_BAR_X, HEALTH_BAR_Y, BAR_WIDTH, BAR_HEIGHT, HUB75_WHITE);

    //bar
    hub75_fill_rect(HEALTH_BAR_X, HEALTH_BAR_Y, displayHealth, BAR_HEIGHT, HUB75_RED);
}

void draw_mana_bar(int totalMana) {
    draw_icon(MANA_ICON, ICON_X, MANA_BAR_Y, HUB75_BLUE);

    int displayMana = totalMana * BAR_WIDTH / MAX_MANA;
    if(displayMana == 0 && totalMana > 0) {
        displayMana = 1;
    }

    //background
    hub75_fill_rect(MANA_BAR_X, MANA_BAR_Y, BAR_WIDTH, BAR_HEIGHT, HUB75_WHITE);

    //bar
    hub75_fill_rect(MANA_BAR_X, MANA_BAR_Y, displayMana, BAR_HEIGHT, HUB75_BLUE);
}

void draw_shield_bar(int totalShieldHealth, int maxShieldHealth, bool shieldActive) {
    draw_icon(SHIELD_ICON, ICON_X, SHIELD_BAR_Y, HUB75_CYAN);
    if(!shieldActive) {
        return;
    }
    int displayShield = totalShieldHealth * BAR_WIDTH / maxShieldHealth;
    if(displayShield == 0 && totalShieldHealth > 0) {
        displayShield = 1;
    }

    //background
    hub75_fill_rect(SHIELD_BAR_X, SHIELD_BAR_Y, BAR_WIDTH, BAR_HEIGHT, HUB75_WHITE);

    //bar
    hub75_fill_rect(SHIELD_BAR_X, SHIELD_BAR_Y, displayShield, BAR_HEIGHT, HUB75_CYAN);

}


void draw_combo(int x, int y, Spell spell, int currentCombo) {

    for(int i = 0; i < spell.comboLength; i++) {
        hub75_color_t color = i < currentCombo ? HUB75_GREEN : HUB75_WHITE;
        if(spell.combo[i] == up) {
            hub75_draw_char_small(upArrow, x, y, color, HUB75_BLACK);
        } else if(spell.combo[i] == down) {
            hub75_draw_char_small(downArrow, x, y, color, HUB75_BLACK);
        } else if(spell.combo[i] == right) {
            hub75_draw_char_small(rightArrow, x, y, color, HUB75_BLACK);
        } else if(spell.combo[i] == left) {
            hub75_draw_char_small(leftArrow, x, y, color, HUB75_BLACK);
        }

        x += HUB75_SMALL_CHAR_WIDTH;
    }

}

void draw_spell_name(int x, int y, char* spell_name) {
    hub75_draw_text_small(spell_name, x, y, HUB75_WHITE, HUB75_BLACK);
}

// void draw_spell(int x, int y, Spell spell) {
//     draw_spell_name(x, y, spell);
//     draw_combo(x, y + COMBO_OFFSET, spell);
// }


void draw_opponent_health_bar(int totalHealth) {
    int displayHealth = totalHealth * OPPONENT_BAR_WIDTH / MAX_HEALTH;
    if(displayHealth == 0 && totalHealth > 0) {
        displayHealth = 1;
    }

    //background
    hub75_fill_rect(OPPONENT_HEALTH_BAR_X, OPPONENT_HEALTH_BAR_Y, OPPONENT_BAR_WIDTH, OPPONENT_BAR_HEIGHT, HUB75_WHITE);

    //bar
    hub75_fill_rect(OPPONENT_HEALTH_BAR_X, OPPONENT_HEALTH_BAR_Y, displayHealth, OPPONENT_BAR_HEIGHT, HUB75_RED);

}

// void draw_opponent_mana_bar(int totalMana) {
//     int displayMana = totalMana * OPPONENT_BAR_WIDTH / MAX_MANA;
//     if(displayMana == 0 && totalMana > 0) {
//         displayMana = 1;
//     }

//     //background
//     hub75_fill_rect(OPPONENT_MANA_BAR_X, OPPONENT_MANA_BAR_Y, OPPONENT_BAR_WIDTH, OPPONENT_BAR_HEIGHT, HUB75_WHITE);

//     //bar
//     hub75_fill_rect(OPPONENT_MANA_BAR_X, OPPONENT_MANA_BAR_Y, displayMana, OPPONENT_BAR_HEIGHT, HUB75_BLUE);
// }

void draw_opponent_shield_bar(int totalShieldHealth, int maxShieldHealth, bool shieldActive) {
    if(!shieldActive) {
        return;
    }
    int displayShield = totalShieldHealth * OPPONENT_BAR_WIDTH / maxShieldHealth;
    if(displayShield == 0 && totalShieldHealth > 0) {
        displayShield = 1;
    }

    //background
    hub75_fill_rect(OPPONENT_SHIELD_BAR_X, OPPONENT_SHIELD_BAR_Y, OPPONENT_BAR_WIDTH, OPPONENT_BAR_HEIGHT, HUB75_WHITE);

    //bar
    hub75_fill_rect(OPPONENT_SHIELD_BAR_X, OPPONENT_SHIELD_BAR_Y, displayShield, OPPONENT_BAR_HEIGHT, HUB75_CYAN);

}

void draw_spell_info(struct Player* player) {
    struct longestCombo combo = findLongestMatchingSpell(player);
    int attackLength = 0;
    int shieldLength = 0;
    int healLength = 0;

    if(combo.spell.type == ATTACK) {
        attackLength = combo.length;
    } else if(combo.spell.type == SHIELD) {
        shieldLength = combo.length;
    } else if(combo.spell.type == HEAL) {
        healLength = combo.length;
    }
    draw_spell_name(2, SPELL_INFO_START, "fire");
    draw_combo(2, SPELL_INFO_START + HUB75_SMALL_CHAR_HEIGHT, SPELLS[0], attackLength);

    draw_spell_name(34, SPELL_INFO_START, "shock");
    draw_combo(34, SPELL_INFO_START + HUB75_SMALL_CHAR_HEIGHT, SPELLS[1], attackLength);

    draw_spell_name(2, SPELL_INFO_START + SPELL_INFO_OFFSET, "shield");
    draw_combo(2, SPELL_INFO_START + SPELL_INFO_OFFSET + HUB75_SMALL_CHAR_HEIGHT, SPELLS[2], shieldLength);

    draw_spell_name(34, SPELL_INFO_START + SPELL_INFO_OFFSET, "fort");
    draw_combo(34, SPELL_INFO_START + SPELL_INFO_OFFSET + HUB75_SMALL_CHAR_HEIGHT, SPELLS[3], shieldLength);

    draw_spell_name(2, SPELL_INFO_START + SPELL_INFO_OFFSET*2, "heal");
    draw_combo(2, SPELL_INFO_START + SPELL_INFO_OFFSET*2 + HUB75_SMALL_CHAR_HEIGHT, SPELLS[4], healLength);
}

void draw_enemy_name() {
    hub75_draw_text_small("ENEMY", 34, SPELL_INFO_START + SPELL_INFO_OFFSET * 2, HUB75_WHITE, HUB75_BLACK);
}


void draw_victory_screen(bool userWon) {

    // user won
    if(userWon) {
        hub75_draw_text("YOU WON!:)", 3, 18, HUB75_WHITE, HUB75_BLACK);
        hub75_draw_text_small("Hold reset to", 6, 44, HUB75_WHITE, HUB75_BLACK);
        hub75_draw_text_small("play again", 12, 50, HUB75_WHITE, HUB75_BLACK);

    } else { // user lost
        hub75_draw_text("YOU LOST:(", 3, 18, HUB75_WHITE, HUB75_BLACK);
        hub75_draw_text_small("Hold reset to", 6, 44, HUB75_WHITE, HUB75_BLACK);
        hub75_draw_text_small("play again", 12, 50, HUB75_WHITE, HUB75_BLACK);

    }
}

// Digits in the 5x8 font only use the top 7 rows, so center a 5x7 glyph.
// The panel is 64x64, so an odd-sized glyph can't be exactly centered;
// this leaves the extra pixel on the right and bottom.
#define COUNTDOWN_DIGIT_HEIGHT 7
#define COUNTDOWN_X (((int)HUB75_WIDTH - (int)HUB75_GLYPH_WIDTH) / 2)
#define COUNTDOWN_Y (((int)HUB75_HEIGHT - COUNTDOWN_DIGIT_HEIGHT) / 2)

void draw_countdown_number(char num) {
    hub75_draw_char(num, COUNTDOWN_X, COUNTDOWN_Y, HUB75_WHITE, HUB75_BLACK);
}

void draw_game(struct Player* player1, struct Player* player2) {
    draw_spell_info(player1);
    draw_health_bar(player1->health);
    draw_mana_bar(player1->mana);
    draw_shield_bar(player1->shieldHealth, player1->maxShieldHealth, player1->shieldActive);
    draw_opponent_health_bar(player2->health);
    draw_opponent_shield_bar(player2->shieldHealth, player2->maxShieldHealth, player2->shieldActive);
    draw_enemy_name();
}