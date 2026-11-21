#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // drawn every frame
#endif

#include "scene.h"
#include "anim_assets.h"
#include "fx.h"
#include "hud.h"
#include "sprite.h"
#include "spells.h"
#include "tween.h"

// ---- layout (play area rows 12..56) ----
// our castle, up close, standing on the ground at the bottom
#define OWN_X        17 // 30 wide -> centred on x = 32
#define OWN_Y        35 // rows 35..56
#define OWN_LAUNCH_Y 37 // fireballs leave from the great tower
#define OWN_HIT_Y    44
#define OWN_CLOUD_Y  31 // storm cloud over our castle
// the enemy castle, far away on the hills
#define FAR_X        25 // 15 wide -> centred on x = 32
#define FAR_Y        19 // rows 19..28
#define FAR_LAUNCH_Y 22
#define FAR_HIT_Y    23
#define FAR_CLOUD_Y  15
#define CENTER_X     32

#define HIT_FLASH_MS   150 // castle flashes white when damage lands
#define FIZZLE_MS      500 // failed cast: red symbol over our castle
#define BOLT_MS        250 // lightning visible after the strike
#define SKY_FLASH_MS   60
#define SPARKS_MS      600

static const hub75_color_t FLASH_PALETTE[16] = {
    0, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE,
    HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE, HUB75_WHITE,
};
static const hub75_color_t FIRE_SPARKS[] = {
    HUB75_RGB(15, 15, 6), HUB75_RGB(15, 9, 0), HUB75_RGB(13, 3, 0), HUB75_RGB(5, 1, 0),
};
static const hub75_color_t ZAP_SPARKS[] = {
    HUB75_RGB(15, 15, 15), HUB75_RGB(13, 13, 6), HUB75_RGB(6, 6, 12), HUB75_RGB(2, 2, 5),
};

static int32_t isqrt(int32_t v) {
    int32_t r = 0;
    while ((r + 1) * (r + 1) <= v) {
        ++r;
    }
    return r;
}

// ---- background ----

static void sky(uint32_t now_ms, bool flash) {
    for (int32_t y = PLAY_AREA_Y; y < 34; ++y) {
        const hub75_color_t c = flash ? HUB75_RGB(3, 3, 6) : (y < 24 ? HUB75_RGB(0, 0, 2) : HUB75_RGB(0, 0, 1));
        hub75_fill_rect(0, y, (int32_t)HUB75_WIDTH, 1, c);
    }
    // twinkling stars
    for (uint32_t i = 0; i < 14; ++i) {
        const uint32_t h = fx_hash(77, i);
        const int32_t x = (int32_t)(h % HUB75_WIDTH);
        const int32_t y = PLAY_AREA_Y + (int32_t)((h >> 8) % 14U);
        const uint32_t level = 4 + ((h >> 16) + now_ms / 180U) % 8U; // 4..11
        const uint32_t twinkle = level > 8 ? 16 - level : level;
        hub75_set_pixel(x, y, HUB75_RGB(twinkle, twinkle, twinkle + 2));
    }
    // moon
    hub75_fill_rect(54, 14, 3, 3, HUB75_RGB(12, 12, 8));
    hub75_set_pixel(53, 15, HUB75_RGB(8, 8, 5));
    hub75_set_pixel(57, 15, HUB75_RGB(8, 8, 5));
    hub75_set_pixel(55, 13, HUB75_RGB(8, 8, 5));
    hub75_set_pixel(55, 17, HUB75_RGB(8, 8, 5));
}

static void land_and_hills(void) {
    for (int32_t x = 0; x < (int32_t)HUB75_WIDTH; ++x) {
        // rolling hills behind the far castle
        int32_t ridge = (x + 6) % 22 - 11;
        ridge = ridge < 0 ? -ridge : ridge;
        const int32_t top = 25 + ridge / 3;
        hub75_fill_rect(x, top, 1, 34 - top, HUB75_RGB(0, 2, 1));
        // the valley, then the ground our castle stands on
        hub75_fill_rect(x, 34, 1, 14, HUB75_RGB(0, 1, 0));
        hub75_fill_rect(x, 48, 1, 9, HUB75_RGB(1, 3, 0));
    }
}

// ---- castles and shields ----

static void castle(const anim_sprite_t *sprite, const anim_clip_t *clip, int32_t x, int32_t y,
                   const struct Player *owner, uint32_t now_ms) {
    const bool hit = now_ms - owner->hitMs < HIT_FLASH_MS;
    anim_draw(sprite, clip, now_ms, x, y, hit ? FLASH_PALETTE : NULL, 0);
}

// a shimmering dome over a castle while its shield is up
static void aura(int32_t cx, int32_t base_y, int32_t rx, int32_t ry, uint32_t now_ms) {
    for (int32_t dx = -rx; dx <= rx; ++dx) {
        const int32_t dy = ry * isqrt(rx * rx - dx * dx) / rx;
        const uint32_t phase = ((uint32_t)(dx + rx) + now_ms / 60U) % 8U;
        hub75_set_pixel(cx + dx, base_y - dy, phase < 2 ? HUB75_CYAN : HUB75_RGB(0, 6, 9));
    }
    for (int32_t dy = 0; dy <= ry; ++dy) { // steep sides
        const int32_t dx = rx * isqrt(ry * ry - dy * dy) / ry;
        const uint32_t phase = ((uint32_t)dy + now_ms / 60U) % 8U;
        const hub75_color_t c = phase < 2 ? HUB75_CYAN : HUB75_RGB(0, 6, 9);
        hub75_set_pixel(cx - dx, base_y - dy, c);
        hub75_set_pixel(cx + dx, base_y - dy, c);
    }
}

// ---- spells ----

static void fireball(const effect_t *e, bool outgoing, bool hits_us, uint32_t age) {
    const int32_t from_y = outgoing ? OWN_LAUNCH_Y : FAR_LAUNCH_Y;
    const int32_t to_y = hits_us ? OWN_HIT_Y : FAR_HIT_Y;
    if (age < FIREBALL_FLIGHT_MS) {
        const int32_t t = tween_ease_in(tween_progress(age, FIREBALL_FLIGHT_MS));
        const int32_t y = tween_lerp(from_y, to_y, t);
        // a slight sideways arc so it isn't a dead-straight line
        const int32_t x = CENTER_X + (outgoing ? 1 : -1) * 6 * t * (TWEEN_ONE - t) / (TWEEN_ONE * TWEEN_ONE / 4);
        anim_draw_centered(&ANIM_FIREBALL, ANIM_FIREBALL_FLY, age, x, y, NULL,
                           outgoing ? 0 : ANIM_FLIP_V);
        return;
    }
    const uint32_t since = age - FIREBALL_FLIGHT_MS;
    if (!anim_finished(ANIM_EXPLOSION_BOOM, since)) {
        anim_draw_centered(&ANIM_EXPLOSION, ANIM_EXPLOSION_BOOM, since, CENTER_X, to_y, NULL, 0);
    }
    fx_burst(CENTER_X, to_y, since, SPARKS_MS, e->start_ms, hits_us ? 18 : 10, FIRE_SPARKS, 4);
}

static void lightning(const effect_t *e, bool hits_us, uint32_t age) {
    const int32_t cloud_y = hits_us ? OWN_CLOUD_Y : FAR_CLOUD_Y;
    const int32_t hit_y = hits_us ? OWN_HIT_Y : FAR_HIT_Y;
    const bool charged = age + 120 >= LIGHTNING_STRIKE_MS && age < LIGHTNING_STRIKE_MS + BOLT_MS;
    anim_draw_centered(&ANIM_CLOUD, charged ? ANIM_CLOUD_CHARGED : ANIM_CLOUD_CHURN, age,
                       CENTER_X, cloud_y, NULL, 0);
    if (age >= LIGHTNING_STRIKE_MS) {
        const uint32_t since = age - LIGHTNING_STRIKE_MS;
        if (since < BOLT_MS && (since / 40U) % 3U != 2U) { // flicker
            fx_bolt(CENTER_X, cloud_y + 3, CENTER_X + (int32_t)(e->start_ms % 5U) - 2, hit_y,
                    e->start_ms + since / 40U, HUB75_WHITE, HUB75_RGB(9, 9, 15));
        }
        fx_burst(CENTER_X, hit_y, since, SPARKS_MS, e->start_ms, hits_us ? 14 : 8, ZAP_SPARKS, 4);
    }
}

static bool lightning_just_struck(const game_t *game, uint32_t viewer, uint32_t now_ms) {
    for (int i = 0; i < MAX_EFFECTS; ++i) {
        const effect_t *e = &game->effects[i];
        const uint32_t age = now_ms - e->start_ms;
        if (e->active && e->kind == EFFECT_LIGHTNING && 1U - e->caster == viewer &&
            age >= LIGHTNING_STRIKE_MS && age < LIGHTNING_STRIKE_MS + SKY_FLASH_MS) {
            return true;
        }
    }
    return false;
}

// ---- the scene ----

void scene_draw(const game_t *game, uint32_t viewer, uint32_t now_ms) {
    const struct Player *me = &game->players[viewer];
    const struct Player *enemy = &game->players[1U - viewer];

    sky(now_ms, lightning_just_struck(game, viewer, now_ms));
    land_and_hills();

    castle(&ANIM_CASTLE_FAR, ANIM_CASTLE_FAR_IDLE, FAR_X, FAR_Y, enemy, now_ms);
    if (enemy->shieldActive) {
        aura(CENTER_X, FAR_Y + 9, 9, 11, now_ms);
    }
    castle(&ANIM_CASTLE, ANIM_CASTLE_IDLE, OWN_X, OWN_Y, me, now_ms);
    if (me->shieldActive) {
        aura(CENTER_X, OWN_Y + 21, 17, 23, now_ms);
    }

    for (int i = 0; i < MAX_EFFECTS; ++i) {
        const effect_t *e = &game->effects[i];
        if (!e->active) {
            continue;
        }
        const uint32_t age = now_ms - e->start_ms;
        const bool outgoing = e->caster == viewer;
        if (e->kind == EFFECT_FIREBALL) {
            fireball(e, outgoing, !outgoing, age);
        } else {
            lightning(e, !outgoing, age);
        }
    }

    // a recognised shape that couldn't be cast: red symbol over our castle
    if (me->lastCast && me->lastCastFailed && now_ms - me->lastCastMs < FIZZLE_MS) {
        sprite_draw_scaled_tinted(me->lastCast->symbol, CENTER_X - 5, OWN_Y - 9, 2, HUB75_RED);
    }
}
