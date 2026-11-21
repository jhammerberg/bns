// Internal: framebuffer shared by the drawing code (hub75_fb.c) and whatever
// sends it to the panels (hub75.c on the XMC, sim/hub75_sim.c on a laptop).
// Game code should use hub75.h instead.

#ifndef HUB75_FB_H
#define HUB75_FB_H

#include <stdint.h>
#include "hub75.h"

// one byte per pixel: bit 0 = red, bit 1 = green, bit 2 = blue
// Columns are in shift order: the first 64 columns sent travel through to the
// far end of the chain, so the panel wired to the XMC (panel 0) is last.
extern uint8_t hub75_framebuffer[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];

// first framebuffer column of `panel` (0 = the panel wired to the XMC)
#define HUB75_PANEL_COLUMN(panel) ((HUB75_PANELS - 1U - (panel)) * HUB75_WIDTH)

// clear every panel and select panel 0
void hub75_fb_reset(void);

#endif // HUB75_FB_H
