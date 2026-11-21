// In-game HUD and game screens. Pixels come from assets/sprites.c.
//
// Layout of each 64x64 panel:
//   rows  1-4   health: heart + player bar | connector | enemy bar + skull.
//               One continuous bar; each side drains toward the middle.
//   rows  6-10  mana: icon, 10 segments (one per mana point), point count
//   rows 12-56  play area (free for game effects)
//   rows 58-62  spell row: each spell's symbol, then the shape that casts it

#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include "hub75.h"
#include "player.h"

#define HUD_ICON_SIZE 5 // icons are 5 wide, 4 tall
#define HUD_ICON_X    1
#define HUD_BAR_X     (HUD_ICON_X + HUD_ICON_SIZE + 1)

// health: player half, 2-pixel white connector, enemy half (mirror image)
#define HEALTH_Y          1
#define HEALTH_BAR_HEIGHT 3
#define CONNECTOR_WIDTH   2
#define CONNECTOR_X       ((int)HUB75_WIDTH / 2 - CONNECTOR_WIDTH / 2) // 31-32
#define HALF_BAR_WIDTH    (CONNECTOR_X - HUD_BAR_X)                      // 24
#define ENEMY_BAR_X       (CONNECTOR_X + CONNECTOR_WIDTH)
#define ENEMY_ICON_X      ((int)HUB75_WIDTH - 1 - HUD_ICON_SIZE)

// mana: MANA_POINTS segments, each MANA_PER_POINT mana, then the point count
#define MANA_Y              7
#define MANA_BAR_HEIGHT     3
#define MANA_PER_POINT      10
#define MANA_POINTS         (MAX_MANA / MANA_PER_POINT)
#define MANA_SEGMENT_WIDTH  4 // plus a 1-pixel gap after each segment
#define MANA_BAR_WIDTH      (MANA_POINTS * (MANA_SEGMENT_WIDTH + 1) - 1)  // 49
#define MANA_NUMBER_Y       (MANA_Y - 1) // 5 rows tall, centred on the bar
#define MANA_NUMBER_RIGHT   ((int)HUB75_WIDTH - 1) // last column of the number

// spell row along the bottom: 4 spells, 16 pixels apart
#define SPELL_SYMBOL_SIZE 5
#define SPELL_ROW_Y       ((int)HUB75_HEIGHT - SPELL_SYMBOL_SIZE - 1)
#define SPELL_ROW_X       2
#define SPELL_SPACING     16

// cast card: the last spell's symbol, big, in the middle of the play area
#define CAST_CARD_MS    500
#define CAST_CARD_SCALE 4

// free space between the status rows and the spell row
#define PLAY_AREA_Y      (MANA_NUMBER_Y + 5 + 1)
#define PLAY_AREA_HEIGHT (SPELL_ROW_Y - 1 - PLAY_AREA_Y)

void draw_health_bar(int health);
void draw_mana_bar(int mana); // segments + whole-point count
void draw_opponent_health_bar(int health);

// spell symbols + shapes; a shape is white if affordable, red if not
void draw_spell_row(const struct Player *player);

void draw_victory_screen(bool userWon);
void draw_countdown_number(char num);

// big symbol of the player's last spell for CAST_CARD_MS, red if it fizzled;
// otherwise keeps the play area clear
void draw_cast_card(const struct Player *player, uint32_t now_ms);

// whole in-game HUD for `player`, with `opponent` as the enemy
void draw_game(const struct Player *player, const struct Player *opponent, uint32_t now_ms);

#endif // HUD_H
