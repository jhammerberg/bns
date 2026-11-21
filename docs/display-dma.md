Display: DMA + binary code modulation
=====================================

How the two chained HUB75 panels are driven (`base/src/display/`).

Measured on the board: 119 fps, display 11-14% CPU, 4096 colors (RGB444).
(The previous CPU-driven scan: 24% CPU, 8 colors; commit fc76e52.)

## How it works
- Pixels are RGB444 (`HUB75_RGB(r, g, b)`, 0..15 each). Each row pair is sent
  4 times per frame, once per bit-plane; plane p is lit for base << p.
- `hub75_encode.c`: framebuffer row pair -> 4 planes x 256 PORT5->OMR writes
  (data with CLK low, then CLK high). Two lookup tables, ~4000 cycles per row.
- `hub75_dma.c`: CCU42 timer at 4 MHz -> DMA request line 0 -> GPDMA0 ch0
  copies one plane to PORT5->OMR (2 MHz shift clock, no CPU).
- `hub75.c`: a plane is latched and lit once its shift finished (GPDMA0_0 IRQ)
  and the previous plane went dark (CCU40 one-shot on-timer, one per plane,
  CCU40_0 IRQ). The next plane shifts while this one is lit. Encoding the
  next row runs in a lower-priority software IRQ (CCU43_0).
- Frames swap only before row 0, so a frame is always shown whole.

## Knobs (hub75.c)
- `HUB75_WHITE_ON_US` (20): lit time per row for full white = brightness/power.
- `HUB75_WRITE_HZ` (4 MHz): port writes per second; the shift clock is half.

## Profiling
`hub75_prof_cycles[3]` / `hub75_prof_calls[3]` count cycles and calls for
[DMA done, LEDs off, encode]; read them with the debugger 1 s apart.
The debugger turns off the cycle counter when it disconnects: press RESET
before trusting the stats line again.

## Next, if needed
- OE in hardware: P5.10 is also CCU80.OUT10, so a CCU8 one-shot could drive
  the LED on-time exactly and drop the LEDs-off interrupt.
- Skip encoding unchanged rows (needs a per-row cache, ~128 KB).
- More planes + a gamma table for smoother dark tones.
