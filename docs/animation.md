Animation
=========

  art/*.sprite              sources (one sprite per file)
  tools/gen_sprites.py      -> base/src/assets/anim_assets.{c,h}  (just sprites)
  base/src/anim/anim.{c,h}  clip playback, drawing
  base/src/anim/tween.h     integer easing (TWEEN_ONE = 1024)
  base/src/anim/fx.{c,h}    procedural particles and bolts
  base/src/display/scene.c  castles, sky, spells

Format:

    sprite fireball
    palette 1=F80 2=FF0          # key=RGB444; '.' is transparent; <= 15 colours
    clip fly loop                # loop | once | pingpong
    frame 60                     # ms, optional event=NAME
    ..1..
    .121.
    ..1..

Generates ANIM_FIREBALL, ANIM_FIREBALL_FLY, ANIM_EVENT_<NAME>.
The generated files are committed, so the firmware builds without Python.

Runtime:

- Stateless: everything is drawn from (clip, elapsed ms). No per-sprite state.
- anim_draw(sprite, clip, elapsed, x, y, palette, flags): palette NULL = own;
  pass another to recolour (hit flash). Flags ANIM_FLIP_H / ANIM_FLIP_V.
- anim_event_between(clip, from, to, ev): event fired in [from, to).
- fx_burst / fx_bolt are seeded by the effect start time, so replays match.

Spells: game.c turns attack casts into effects with travel time; damage
lands at impact (FIREBALL_FLIGHT_MS, LIGHTNING_STRIKE_MS). A shield raised
before impact still blocks. Health bars glide over HEALTH_GLIDE_MS.

New sprite: add art/NAME.sprite, save (just watch rebuilds), use
ANIM_NAME in scene.c.
