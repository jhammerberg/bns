// Port of tools/gen_templates.py (stroke_path, resample, normalize, $P).

#include "shape.h"
#include <float.h>
#include <math.h>
#include "shape_templates.h"

#define TRIM_MS    100 // ignore button press/release wobble at each end
#define MIN_POINTS 10  // shorter strokes (after trimming) are rejected
#define RAD2DEG    57.29577951f

const shape_template_t *const shape_templates = SHAPE_TEMPLATES;
const int shape_template_count = sizeof SHAPE_TEMPLATES / sizeof SHAPE_TEMPLATES[0];

void shape_stroke_begin(shape_stroke_t *s) {
    s->count = 0;
}

void shape_stroke_add(shape_stroke_t *s, uint32_t t_ms, float qw, float qx, float qy, float qz) {
    if (s->count >= SHAPE_MAX_SAMPLES) {
        return;
    }
    s->t_ms[s->count] = t_ms;
    s->q[s->count][0] = qw;
    s->q[s->count][1] = qx;
    s->q[s->count][2] = qy;
    s->q[s->count][3] = qz;
    s->count++;
}

// Where the wand tip points: sensor X axis rotated into the world frame
// (Z up), as yaw (relative to the first sample) and pitch, in degrees.
static int stroke_path(const shape_stroke_t *s, shape_point_t *out) {
    if (s->count == 0) {
        return 0;
    }
    const uint32_t t0 = s->t_ms[0], t1 = s->t_ms[s->count - 1];
    int n = 0;
    float yaw0 = 0;
    for (int pass = 0; pass < 2 && n == 0; ++pass) {
        for (int i = 0; i < s->count; ++i) {
            // pass 0: trimmed; pass 1 (only if nothing survived): everything
            if (pass == 0 && (s->t_ms[i] < t0 + TRIM_MS || s->t_ms[i] + TRIM_MS > t1)) {
                continue;
            }
            const float w = s->q[i][0], x = s->q[i][1], y = s->q[i][2], z = s->q[i][3];
            const float vx = 1 - 2 * (y * y + z * z);
            const float vy = 2 * (x * y + w * z);
            float vz = 2 * (x * z - w * y);
            vz = vz > 1 ? 1 : (vz < -1 ? -1 : vz);
            const float yaw = atan2f(vy, vx) * RAD2DEG;
            const float pitch = asinf(vz) * RAD2DEG;
            if (n == 0) {
                yaw0 = yaw;
            }
            float rel = fmodf(yaw - yaw0 + 180.0f, 360.0f);
            if (rel < 0) {
                rel += 360.0f;
            }
            rel -= 180.0f;
            out[n++] = (shape_point_t){-rel, pitch}; // flip so x grows to the right
        }
    }
    return n;
}

static float dist(shape_point_t a, shape_point_t b) { // used by resample()
    return hypotf(a.x - b.x, a.y - b.y);
}

static void resample(const shape_point_t *in, int n_in, shape_point_t out[SHAPE_POINTS]) {
    float length = 0;
    for (int i = 1; i < n_in; ++i) {
        length += dist(in[i - 1], in[i]);
    }
    const float interval = length > 0 ? length / (SHAPE_POINTS - 1) : 1.0f;
    int k = 0;
    out[k++] = in[0];
    shape_point_t prev = in[0];
    float D = 0;
    for (int i = 1; i < n_in; ++i) {
        const shape_point_t cur = in[i];
        float d = dist(prev, cur);
        while (D + d >= interval && d > 0 && k < SHAPE_POINTS) {
            const float t = (interval - D) / d;
            prev = (shape_point_t){prev.x + t * (cur.x - prev.x), prev.y + t * (cur.y - prev.y)};
            out[k++] = prev;
            d = dist(prev, cur);
            D = 0;
        }
        D += d;
        prev = cur;
    }
    while (k < SHAPE_POINTS) {
        out[k++] = in[n_in - 1];
    }
}

bool shape_normalize_stroke(const shape_stroke_t *s, shape_point_t out[SHAPE_POINTS]) {
    static shape_point_t path[SHAPE_MAX_SAMPLES];
    const int n = stroke_path(s, path);
    if (n < MIN_POINTS) {
        return false;
    }
    resample(path, n, out);

    float minx = FLT_MAX, maxx = -FLT_MAX, miny = FLT_MAX, maxy = -FLT_MAX;
    for (int i = 0; i < SHAPE_POINTS; ++i) {
        minx = fminf(minx, out[i].x); maxx = fmaxf(maxx, out[i].x);
        miny = fminf(miny, out[i].y); maxy = fmaxf(maxy, out[i].y);
    }
    float scale = fmaxf(maxx - minx, maxy - miny);
    if (scale == 0) {
        scale = 1;
    }
    float cx = 0, cy = 0;
    for (int i = 0; i < SHAPE_POINTS; ++i) {
        out[i].x = (out[i].x - minx) / scale;
        out[i].y = (out[i].y - miny) / scale;
        cx += out[i].x;
        cy += out[i].y;
    }
    cx /= SHAPE_POINTS;
    cy /= SHAPE_POINTS;
    for (int i = 0; i < SHAPE_POINTS; ++i) {
        out[i].x -= cx;
        out[i].y -= cy;
    }
    return true;
}

// ---- $P matching in integer arithmetic ----
// The ESP32-C6 has no FPU, so float maths is emulated in software (~6 s per
// stroke). Points are scaled to integers (normalised coordinates are within
// +-1, so x4096 fits int16), squared distances pick the nearest point, and
// only the chosen distance is square-rooted. Scores are compared with an early
// abandon against the best template so far.

#define FIX_SCALE 4096

typedef struct {
    int16_t x, y;
} fix_point_t;

static void to_fixed(const shape_point_t *in, fix_point_t *out) {
    for (int i = 0; i < SHAPE_POINTS; ++i) {
        out[i].x = (int16_t)lrintf(in[i].x * FIX_SCALE);
        out[i].y = (int16_t)lrintf(in[i].y * FIX_SCALE);
    }
}

static uint32_t isqrt32(uint32_t x) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= r + bit) {
            x -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

// Weighted sum of nearest-point distances, in FIX_SCALE units times
// SHAPE_POINTS (the $P weight 1 - k/n is applied as n - k). Stops early once
// the sum passes `limit`.
static uint32_t cloud_distance(const fix_point_t *a, const fix_point_t *b, int start, uint32_t limit) {
    bool matched[SHAPE_POINTS] = {false};
    uint32_t total = 0;
    int i = start, k = 0;
    do {
        uint32_t best = UINT32_MAX;
        int idx = 0;
        for (int j = 0; j < SHAPE_POINTS; ++j) {
            if (!matched[j]) {
                const int32_t dx = a[i].x - b[j].x, dy = a[i].y - b[j].y;
                const uint32_t d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (d2 < best) {
                    best = d2;
                    idx = j;
                }
            }
        }
        matched[idx] = true;
        total += (uint32_t)(SHAPE_POINTS - k) * isqrt32(best);
        if (total >= limit) {
            return total;
        }
        i = (i + 1) % SHAPE_POINTS;
        ++k;
    } while (i != start);
    return total;
}

static uint32_t greedy_match(const fix_point_t *a, const fix_point_t *b, uint32_t limit) {
    const int step = 5; // floor(sqrt(SHAPE_POINTS))
    uint32_t best = limit;
    for (int i = 0; i < SHAPE_POINTS; i += step) {
        uint32_t d = cloud_distance(a, b, i, best);
        if (d < best) best = d;
        d = cloud_distance(b, a, i, best);
        if (d < best) best = d;
    }
    return best;
}

static float to_distance(uint32_t fixed_total) {
    return (float)fixed_total / ((float)FIX_SCALE * SHAPE_POINTS);
}

shape_result_t shape_classify(const shape_point_t pts[SHAPE_POINTS],
                              const shape_template_t *templates, int count, int skip_index) {
    fix_point_t a[SHAPE_POINTS], b[SHAPE_POINTS];
    to_fixed(pts, a);
    uint32_t best = UINT32_MAX;
    shape_result_t r = {BNS_SHAPE_NONE, BNS_SHAPE_NONE, FLT_MAX};
    for (int i = 0; i < count; ++i) {
        if (i == skip_index) {
            continue;
        }
        to_fixed(templates[i].pts, b);
        const uint32_t d = greedy_match(a, b, best);
        if (d < best) {
            best = d;
            r.best = templates[i].shape;
        }
    }
    if (best != UINT32_MAX) {
        r.distance = to_distance(best);
    }
    r.shape = r.distance <= SHAPE_REJECT_DISTANCE ? r.best : BNS_SHAPE_NONE;
    return r;
}

shape_result_t shape_recognize(const shape_stroke_t *s) {
    shape_point_t pts[SHAPE_POINTS];
    if (!shape_normalize_stroke(s, pts)) {
        return (shape_result_t){BNS_SHAPE_NONE, BNS_SHAPE_NONE, FLT_MAX};
    }
    return shape_classify(pts, shape_templates, shape_template_count, -1);
}

const char *shape_name(bns_shape_t shape) {
    switch (shape) {
    case BNS_SHAPE_NONE: return "none";
    case BNS_SHAPE_TRIANGLE: return "triangle";
    case BNS_SHAPE_ZIGZAG: return "zigzag";
    case BNS_SHAPE_CIRCLE: return "circle";
    case BNS_SHAPE_V: return "v";
    case BNS_SHAPE_COUNT: break;
    }
    return "?";
}
