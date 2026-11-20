// Internal: framebuffer shared by the drawing code (hub75_fb.c) and whatever
// sends it to the panels (hub75.c on the XMC, sim/hub75_sim.c on a laptop).
// Game code should use hub75.h instead.

#ifndef HUB75_FB_H
#define HUB75_FB_H

#include <stdint.h>
#include "hub75.h"

// one byte per pixel: bit 0 = red, bit 1 = green, bit 2 = blue
// columns 0..63 are panel 0, 64..127 panel 1, and so on down the chain
extern uint8_t hub75_framebuffer[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];

// clear every panel and select panel 0
void hub75_fb_reset(void);

#endif // HUB75_FB_H
