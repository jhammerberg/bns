#include "hub75.h"
#include "hub75_text.h"
#include "spells.h"
#include "game_assets.h"

void draw_health(int x, int y) {
    hub75_draw_text("HEALTH", HEALTH_NAME_X, HEALTH_NAME_Y, HUB75_WHITE, HUB75_BLACK);
}

void draw_mana(int x, int y) {
    hub75_draw_text("MANA", MANA_NAME_X, MANA_NAME_Y, HUB75_WHITE, HUB75_BLACK);
}

void draw_health_bar(int totalHealth) {
    int displayHealth = totalHealth / BAR_WIDTH;
    if(displayHealth == 0 && totalHealth > 0) {
        displayHealth = 1;
    }
    hub75_fill_rect(HEALTH_BAR_X, HEALTH_BAR_Y, displayHealth, BAR_HEIGHT, HUB75_RED);
}

void draw_mana_bar(int totalMana) {
    int displayMana = totalMana / BAR_WIDTH;
    if(displayMana == 0 && totalMana > 0) {
        displayMana = 1;
    }
    hub75_fill_rect(MANA_BAR_X, MANA_BAR_Y, displayMana, BAR_HEIGHT, HUB75_BLUE);
}

void draw_combo(int x, int y, Spell spell) {
    for(int i = 0; i < spell.comboLength; i++) {
        if(spell.combo[i] == up) {
            hub75_draw_char(upArrow, x, y, HUB75_WHITE, HUB75_BLACK);
        } else if(spell.combo[i] == down) {
            hub75_draw_char(downArrow, x, y, HUB75_WHITE, HUB75_BLACK);
        } else if(spell.combo[i] == right) {
            hub75_draw_char(rightArrow, x, y, HUB75_WHITE, HUB75_BLACK);
        } else if(spell.combo[i] == left) {
            hub75_draw_char(leftArrow, x, y, HUB75_WHITE, HUB75_BLACK);
        }

        x += 4;
    }

}

void draw_spell_name(int x, int y, Spell spell) {
    hub75_draw_text(spell.spellName, x, y, HUB75_WHITE, HUB75_BLACK);
}

#define COMBO_OFFSET 5
void draw_spell(int x, int y, Spell spell) {
    draw_spell_name(x, y, spell);
    draw_combo(x, y + COMBO_OFFSET, spell);
}