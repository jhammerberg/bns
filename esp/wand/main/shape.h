// Shape recognition for the wand: orientation samples while the draw button
// is held -> 2D path of where the tip points -> $P point-cloud recogniser.
// Plain C (no ESP-IDF), so test/ can run it on a laptop against captures/.
// Reference implementation: tools/gen_templates.py -- keep them in step.

#ifndef SHAPE_H
#define SHAPE_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define SHAPE_POINTS      32   // points per normalised stroke ($P "n")
#define SHAPE_MAX_SAMPLES 500  // 5 s at 100 Hz; longer strokes are cut off

typedef struct {
    float x, y;
} shape_point_t;

typedef struct {
    bns_shape_t shape;
    shape_point_t pts[SHAPE_POINTS]; // normalised
} shape_template_t;

typedef struct {
    uint32_t t_ms[SHAPE_MAX_SAMPLES];
    float q[SHAPE_MAX_SAMPLES][4]; // w, x, y, z
    int count;
} shape_stroke_t;

typedef struct {
    bns_shape_t shape; // BNS_SHAPE_NONE if too short or not close enough
    bns_shape_t best;  // closest template's shape, even if rejected
    float distance;    // $P distance to the closest template (lower = better)
} shape_result_t;

void shape_stroke_begin(shape_stroke_t *s);
void shape_stroke_add(shape_stroke_t *s, uint32_t t_ms, float qw, float qx, float qy, float qz);

// Recognise a finished stroke against the built-in templates.
shape_result_t shape_recognize(const shape_stroke_t *s);

// Lower-level pieces, used by test/ for leave-one-out checks.
// Returns false if the stroke is too short to recognise.
bool shape_normalize_stroke(const shape_stroke_t *s, shape_point_t out[SHAPE_POINTS]);
shape_result_t shape_classify(const shape_point_t pts[SHAPE_POINTS],
                              const shape_template_t *templates, int count, int skip_index);

const char *shape_name(bns_shape_t shape);

extern const shape_template_t *const shape_templates;
extern const int shape_template_count;

#endif // SHAPE_H
