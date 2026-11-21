// Drawing sprites from assets/sprites.c onto the display.

#ifndef SPRITE_H
#define SPRITE_H

#include "hub75.h"
#include "sprites.h"

// draw in the sprite's own colours, top-left corner at (x,y)
void sprite_draw(const sprite_t *sprite, int32_t x, int32_t y);

// draw every lit pixel in one colour (used for the wand shape icons)
void sprite_draw_tinted(const sprite_t *sprite, int32_t x, int32_t y, hub75_color_t color);

// draw `scale` times bigger (each pixel becomes a scale x scale block)
void sprite_draw_scaled(const sprite_t *sprite, int32_t x, int32_t y, int scale);
void sprite_draw_scaled_tinted(const sprite_t *sprite, int32_t x, int32_t y, int scale,
                               hub75_color_t color);

int sprite_width(const sprite_t *sprite);
int sprite_height(const sprite_t *sprite);

#endif // SPRITE_H
