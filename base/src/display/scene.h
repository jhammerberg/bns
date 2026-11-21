// The play area (between the health/mana rows and the spell row) as seen by
// one player: night sky, the enemy castle far away at the top, our castle up
// close at the bottom, and the spells flying between them.
//
// Everything is drawn from the game state and the time, so the scene has no
// state of its own (see anim/anim.h).

#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>
#include "game.h"

// draw player `viewer`'s (0 or 1) view of the play area into the back buffer
void scene_draw(const game_t *game, uint32_t viewer, uint32_t now_ms);

#endif // SCENE_H
