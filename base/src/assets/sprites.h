// All sprites (small images) used by the game. Data only: the pixels live in
// sprites.c, drawing code is display/sprite.c.
//
// Format: one string per row, one character per pixel.
//   R red   G green   B blue   Y yellow   C cyan   M magenta   W white
//   .  off (drawn black)      space  transparent (left as is)
// Rows must all be the same length; at most SPRITE_MAX_ROWS rows.

#ifndef SPRITES_H
#define SPRITES_H

#include <stddef.h>
#include "protocol.h"

#define SPRITE_MAX_ROWS 8

typedef struct {
    const char *rows[SPRITE_MAX_ROWS + 1]; // unused rows are NULL
} sprite_t;

// HUD icons (5x4)
extern const sprite_t SPRITE_HEART;
extern const sprite_t SPRITE_MANA;     // mana drop (5x5)
extern const sprite_t SPRITE_SKULL;    // enemy health

// spell symbols (5x5)
extern const sprite_t SPRITE_FIRE;
extern const sprite_t SPRITE_LIGHTNING;
extern const sprite_t SPRITE_SHIELD;
extern const sprite_t SPRITE_HEAL;

// wand shapes (5x5, drawn tinted), indexed by bns_shape_t
extern const sprite_t *const SPRITE_SHAPES[BNS_SHAPE_COUNT];

#endif // SPRITES_H
