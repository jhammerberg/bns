// Sprite pixels. Format and colour letters: see sprites.h.

#include "sprites.h"

// ---- HUD icons (5x4) ----

const sprite_t SPRITE_HEART = {{
    "RR.RR",
    "RRRRR",
    ".RRR.",
    "..R..",
}};

// mana drop (5x5)
const sprite_t SPRITE_MANA = {{
    "..B..",
    "..B..",
    ".BBB.",
    "BWBBB",
    ".BBB.",
}};

const sprite_t SPRITE_SKULL = {{
    ".WWW.",
    "W.W.W",
    "WWWWW",
    ".W.W.",
}};

// ---- spell symbols (5x5) ----

const sprite_t SPRITE_FIRE = {{
    "..R..",
    ".RR.R",
    "RRYRR",
    "RYYYR",
    ".RYR.",
}};

const sprite_t SPRITE_LIGHTNING = {{
    "...YY",
    "..YY.",
    ".YYYY",
    "..YY.",
    ".YY..",
}};

const sprite_t SPRITE_SHIELD = {{
    "CCCCC",
    "CWCCC",
    "CCCCC",
    ".CCC.",
    "..C..",
}};

const sprite_t SPRITE_HEAL = {{
    "..G..",
    "..G..",
    "GGGGG",
    "..G..",
    "..G..",
}};

// ---- wand shapes (5x5); drawn with a tint, so the letter doesn't matter ----

static const sprite_t SHAPE_TRIANGLE = {{
    "..W..",
    ".W.W.",
    ".W.W.",
    "W...W",
    "WWWWW",
}};

static const sprite_t SHAPE_ZIGZAG = {{
    "WWWWW",
    "...W.",
    "..W..",
    ".W...",
    "WWWWW",
}};

static const sprite_t SHAPE_CIRCLE = {{
    ".WWW.",
    "W...W",
    "W...W",
    "W...W",
    ".WWW.",
}};

static const sprite_t SHAPE_V = {{
    "W...W",
    "W...W",
    ".W.W.",
    ".W.W.",
    "..W..",
}};

const sprite_t *const SPRITE_SHAPES[BNS_SHAPE_COUNT] = {
    [BNS_SHAPE_NONE] = NULL,
    [BNS_SHAPE_TRIANGLE] = &SHAPE_TRIANGLE,
    [BNS_SHAPE_ZIGZAG] = &SHAPE_ZIGZAG,
    [BNS_SHAPE_CIRCLE] = &SHAPE_CIRCLE,
    [BNS_SHAPE_V] = &SHAPE_V,
};
