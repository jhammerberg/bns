#include "hud.h"
#include "hub75_text.h"
#include "spells.h"
#include "sprite.h"

// Bar showing value/max: white background, `color` fill. Any value above 0
// shows at least one pixel so it never looks empty when it isn't.
// from_right: fill grows from the right end (used for the mirrored enemy bar).
static void draw_bar(int x, int y, int width, int height, int value, int max,
                     hub75_color_t color, bool from_right) {
    int filled = max > 0 ? value * width / max : 0;
    if (filled == 0 && value > 0) {
        filled = 1;
    }
    hub75_fill_rect(x, y, width, height, HUB75_WHITE);
    hub75_fill_rect(from_right ? x + width - filled : x, y, filled, height, color);
}

void draw_health_bar(int health) {
    sprite_draw(&SPRITE_HEART, HUD_ICON_X, HEALTH_Y);
    draw_bar(HUD_BAR_X, HEALTH_Y, HALF_BAR_WIDTH, HEALTH_BAR_HEIGHT,
             health, MAX_HEALTH, HUB75_RED, false);
    hub75_fill_rect(CONNECTOR_X, HEALTH_Y, CONNECTOR_WIDTH, HEALTH_BAR_HEIGHT, HUB75_WHITE);
}

void draw_opponent_health_bar(int health) {
    sprite_draw(&SPRITE_SKULL, ENEMY_ICON_X, HEALTH_Y);
    draw_bar(ENEMY_BAR_X, HEALTH_Y, HALF_BAR_WIDTH, HEALTH_BAR_HEIGHT,
             health, MAX_HEALTH, HUB75_RED, true);
}

// Clash Royale style: one segment per mana point, black gaps between them.
// Full points are blue, the point being regenerated fills partly, the rest
// are white. The whole-point count is drawn at the right edge.
void draw_mana_bar(int mana) {
    sprite_draw(&SPRITE_MANA, HUD_ICON_X, MANA_NUMBER_Y); // 5 tall, lines up with the number
    hub75_fill_rect(HUD_BAR_X, MANA_Y, MANA_BAR_WIDTH, MANA_BAR_HEIGHT, HUB75_BLACK);
    for (int i = 0; i < MANA_POINTS; i++) {
        const int x = HUD_BAR_X + i * (MANA_SEGMENT_WIDTH + 1);
        int in_segment = mana - i * MANA_PER_POINT; // mana that falls in this segment
        if (in_segment < 0) in_segment = 0;
        if (in_segment > MANA_PER_POINT) in_segment = MANA_PER_POINT;
        const int filled = in_segment * MANA_SEGMENT_WIDTH / MANA_PER_POINT;
        hub75_fill_rect(x, MANA_Y, MANA_SEGMENT_WIDTH, MANA_BAR_HEIGHT, HUB75_WHITE);
        hub75_fill_rect(x, MANA_Y, filled, MANA_BAR_HEIGHT, HUB75_BLUE);
    }

    // point count, right-aligned (one or two 3x5 digits)
    const int points = mana / MANA_PER_POINT;
    const int number_width = 2 * (int)HUB75_SMALL_CHAR_WIDTH - 1; // room for "10"
    hub75_fill_rect(MANA_NUMBER_RIGHT - number_width + 1, MANA_NUMBER_Y, number_width,
                    (int)HUB75_SMALL_GLYPH_HEIGHT, HUB75_BLACK);
    char text[3] = {0};
    if (points >= 10) {
        text[0] = (char)('0' + points / 10 % 10);
        text[1] = (char)('0' + points % 10);
    } else {
        text[0] = (char)('0' + (points < 0 ? 0 : points));
    }
    const int width = (int)(sizeof text - 1 - (text[1] == 0)) * (int)HUB75_SMALL_CHAR_WIDTH - 1;
    hub75_draw_text_small(text, MANA_NUMBER_RIGHT - width + 1, MANA_NUMBER_Y, HUB75_WHITE, HUB75_BLACK);
}

void draw_spell_row(const struct Player *player) {
    for (size_t i = 0; i < NUM_SPELLS; i++) {
        const int x = SPELL_ROW_X + (int)i * SPELL_SPACING;
        const struct Spell *spell = &SPELLS[i];
        sprite_draw(spell->symbol, x, SPELL_ROW_Y);
        // shape icon just to the right of the symbol
        const hub75_color_t color = player->mana >= spell->manaCost ? HUB75_WHITE : HUB75_RED;
        sprite_draw_tinted(SPRITE_SHAPES[spell->shape], x + SPELL_SYMBOL_SIZE + 1, SPELL_ROW_Y, color);
    }
}

void draw_victory_screen(bool userWon) {
    hub75_draw_text(userWon ? "YOU WON!:)" : "YOU LOST:(", 3, 18, HUB75_WHITE, HUB75_BLACK);
    hub75_draw_text_small("Cast to", 18, 44, HUB75_WHITE, HUB75_BLACK);
    hub75_draw_text_small("play again", 12, 50, HUB75_WHITE, HUB75_BLACK);
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

void draw_cast_card(const struct Player *player, uint32_t now_ms) {
    hub75_fill_rect(0, PLAY_AREA_Y, (int)HUB75_WIDTH, PLAY_AREA_HEIGHT, HUB75_BLACK);
    if(player->lastCast == NULL || now_ms - player->lastCastMs >= CAST_CARD_MS) {
        return;
    }
    const sprite_t *symbol = player->lastCast->symbol;
    const int w = sprite_width(symbol) * CAST_CARD_SCALE;
    const int h = sprite_height(symbol) * CAST_CARD_SCALE;
    const int x = ((int)HUB75_WIDTH - w) / 2;
    const int y = PLAY_AREA_Y + (PLAY_AREA_HEIGHT - h) / 2;
    if(player->lastCastFailed) {
        sprite_draw_scaled_tinted(symbol, x, y, CAST_CARD_SCALE, HUB75_RED);
    } else {
        sprite_draw_scaled(symbol, x, y, CAST_CARD_SCALE);
    }
}

void draw_game(const struct Player *player, const struct Player *opponent, uint32_t now_ms) {
    draw_health_bar(player->health);
    draw_opponent_health_bar(opponent->health);
    draw_mana_bar(player->mana);
    draw_spell_row(player);
    draw_cast_card(player, now_ms);
}
