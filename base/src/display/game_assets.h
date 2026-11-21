#include "spells.h"
#include "player.h"
#include "hub75_text.h"

#define HEALTH_NAME_X 0
#define HEALTH_NAME_Y 10

#define MANA_NAME_X 0
#define MANA_NAME_Y 20

// Player bars: a 5x4 icon at the left, then the bar after a 1-pixel gap.
// Each icon starts one row above its bar so it lines up with the bar's bottom.
#define ICON_X 2
#define ICON_WIDTH 5
#define ICON_HEIGHT 4
#define BAR_X (ICON_X + ICON_WIDTH + 1) // 6
#define BAR_WIDTH (64-4-(ICON_WIDTH+1))                  // 6 + 58 = 64, the panel's right edge
#define BAR_HEIGHT 4

#define HEALTH_BAR_X BAR_X
#define HEALTH_BAR_Y 2

#define MANA_BAR_X BAR_X
#define MANA_BAR_Y (HEALTH_BAR_Y + BAR_HEIGHT + 2)

// No icon for the shield; it shares BAR_X so all three bars line up.
#define SHIELD_BAR_X BAR_X
#define SHIELD_BAR_Y (MANA_BAR_Y + BAR_HEIGHT + 2)




#define SPELL_INFO_START (SHIELD_BAR_Y + BAR_HEIGHT + 3)
#define SPELL_INFO_OFFSET (HUB75_SMALL_CHAR_HEIGHT + 1 + HUB75_SMALL_GLYPH_HEIGHT + 2)

#define OPPONENT_BAR_WIDTH 28 
#define OPPONENT_BAR_HEIGHT 2

#define OPPONENT_HEALTH_BAR_X 34
#define OPPONENT_HEALTH_BAR_Y (SPELL_INFO_START + SPELL_INFO_OFFSET * 2 + HUB75_SMALL_CHAR_HEIGHT)

#define OPPONENT_SHIELD_BAR_X 34
#define OPPONENT_SHIELD_BAR_Y (OPPONENT_HEALTH_BAR_Y + OPPONENT_BAR_HEIGHT + 1)


void draw_health(int x, int y);

void draw_mana(int x, int y);

void draw_health_bar(int totalHealth);

void draw_mana_bar(int totalMana);

void draw_shield_bar(int totalShieldHealth, int maxShieldHealth, bool shieldActive);

void draw_enemy_name();

void draw_opponent_health_bar(int totalHealth);

void draw_opponent_mana_bar(int totalMana);

void draw_opponent_shield_bar(int totalShieldHealth, int maxShieldHealth, bool shieldActive);

#define upArrow 0x18
#define downArrow 0x19
#define rightArrow 0x1A
#define leftArrow 0x1B

void draw_combo(int x, int y, Spell spell, int currentCombo);

void draw_spell_name(int x, int y, char* spell_name);

void draw_spell_info(struct Player* player);


#define COMBO_OFFSET ((int)HUB75_SMALL_CHAR_HEIGHT + 1)
void draw_spell(int x, int y, Spell spell);

void draw_victory_screen(bool userWon);

void draw_countdown_number(char num);

void draw_game(struct Player* player1, struct Player* player2);