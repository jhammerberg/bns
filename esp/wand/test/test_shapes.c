// Laptop test for main/shape.c against the recordings in captures/.
//   make -C test        (from esp/wand)
// 1. Each recorded stroke, processed by the C code, must match the template
//    tools/gen_templates.py made from it (checks the port is faithful).
// 2. Leave-one-out: each stroke vs all the other templates must be recognised.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shape.h"

static shape_stroke_t stroke;

static bns_shape_t parse_label(const char *s) {
    for (int i = 1; i < BNS_SHAPE_COUNT; ++i) {
        if (strcmp(s, shape_name((bns_shape_t)i)) == 0) return (bns_shape_t)i;
    }
    return BNS_SHAPE_NONE; // e.g. "square": not one of the game's shapes
}

static int checked, mismatched, correct, rejected;
static float worst_diff;

static void finish_stroke(bns_shape_t label, int mixed) {
    if (stroke.count == 0 || mixed || label == BNS_SHAPE_NONE) return;
    shape_point_t pts[SHAPE_POINTS];
    if (!shape_normalize_stroke(&stroke, pts)) return; // generator skips these too
    const int idx = checked++;
    if (idx >= shape_template_count || shape_templates[idx].shape != label) {
        printf("stroke %d: template order/label mismatch\n", idx);
        mismatched++;
        return;
    }
    for (int i = 0; i < SHAPE_POINTS; ++i) {
        worst_diff = fmaxf(worst_diff, fabsf(pts[i].x - shape_templates[idx].pts[i].x));
        worst_diff = fmaxf(worst_diff, fabsf(pts[i].y - shape_templates[idx].pts[i].y));
    }
    shape_result_t r = shape_classify(pts, shape_templates, shape_template_count, idx);
    if (r.shape == label) correct++;
    else if (r.shape == BNS_SHAPE_NONE) rejected++;
    else printf("stroke %d: %s recognised as %s (d=%.3f)\n", idx, shape_name(label),
                shape_name(r.shape), r.distance);
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "../captures/session1.csv";
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 1; }
    char line[256], label[32] = "", cur_label[32] = "";
    int cur_stroke = -1, mixed = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || strncmp(line, "label,", 6) == 0) continue;
        int id; unsigned t; float w, x, y, z;
        if (sscanf(line, "%31[^,],%d,%u,%f,%f,%f,%f", label, &id, &t, &w, &x, &y, &z) != 7) continue;
        if (id != cur_stroke) {
            finish_stroke(parse_label(cur_label), mixed);
            shape_stroke_begin(&stroke);
            cur_stroke = id; mixed = 0;
            strcpy(cur_label, label);
        } else if (strcmp(label, cur_label) != 0) {
            mixed = 1;
        }
        shape_stroke_add(&stroke, t, w, x, y, z);
    }
    finish_stroke(parse_label(cur_label), mixed);
    fclose(f);

    printf("%d strokes (templates: %d), C vs Python max point difference %.5f\n",
           checked, shape_template_count, worst_diff);
    printf("leave-one-out: %d/%d correct, %d rejected\n", correct, checked, rejected);
    const int ok = checked == shape_template_count && !mismatched && worst_diff < 1e-3f &&
                   correct == checked;
    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
