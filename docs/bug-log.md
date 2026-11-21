Bug log
=======

Newest at the bottom. Symptom, cause, fix.

## 2026-11-21 Panels dark after flashing
- Symptom: both panels blank after `just flash xmc`.
- Cause: shorted contacts in the wiring, not firmware.
- Fix: separated the contacts. Check wiring first.

## 2026-11-21 J-Link "Cannot connect to the probe"
- Symptom: `make program` fails even though the J-Link shows on USB.
- Cause: on-board J-Link stuck.
- Fix: unplug X101, wait 5 s, plug back in.

## 2026-11-21 Player views on the wrong panels
- Symptom: player 1's view on the chained panel, not the XMC-side one.
- Cause: data shifted first ends up in the far panel of the chain.
- Fix: `HUB75_PANEL_COLUMN` in `base/src/display/hub75_fb.h`; panel 0 = XMC-side panel.

## 2026-11-21 Shapes shimmer (heart, start of health bar)
- Symptom: small shapes flicker, solid areas look fine.
- Cause: only one HUB75 ground (pin 4) connected; pin 16 was loose.
- Fix: wire both grounds (pins 4 and 16) to the XMC. Clock softening not needed.

## 2026-11-21 Mana bar and rows 8-15 / 24-31 missing
- Symptom: mana bar gone, other rows fine; white still white.
- Cause: soft pin edges (`HUB75_PIN_STRENGTH`) weakened P0.3 (address bit D, A2 pad).
- Fix: keep HUB75 pins on strong/sharp in `base/src/display/hub75.c`.
  Drive codes mean different things per pad class (XMC4700 RM Table 26-6).

## Quick diagnosis
- White looks yellow: blue data line (B1 pin 3 / B2 pin 7) missing.
- A whole band of rows missing: a row address line (A-E).
- Shapes shimmer, solid areas fine: grounds or clock signal integrity.
