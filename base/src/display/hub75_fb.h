// Internal: framebuffers shared by the drawing code (hub75_fb.c) and whatever
// sends them to the panels (hub75.c on the XMC, sim/hub75_sim.c on a laptop).
// Game code should use hub75.h instead.

#ifndef HUB75_FB_H
#define HUB75_FB_H

#include <stdint.h>
#include "hub75.h"

// one byte per pixel: bit 0 = red, bit 1 = green, bit 2 = blue
// Columns are in shift order: the first 64 columns sent travel through to the
// far end of the chain, so the panel wired to the XMC (panel 0) is last.
typedef uint8_t hub75_frame_t[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];

// Double buffering: drawing goes into the back frame, the panels show the
// front frame. hub75_fb_swap() exchanges them (at a frame boundary).
extern hub75_frame_t *hub75_back;
extern hub75_frame_t *volatile hub75_front;
void hub75_fb_swap(void);

// first framebuffer column of `panel` (0 = the panel wired to the XMC)
#define HUB75_PANEL_COLUMN(panel) ((HUB75_PANELS - 1U - (panel)) * HUB75_WIDTH)

// clear both frames and select panel 0
void hub75_fb_reset(void);

#endif // HUB75_FB_H
