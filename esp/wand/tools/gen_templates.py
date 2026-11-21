#!/usr/bin/env python3
"""Build main/shape_templates.h from the recorded strokes in captures/*.csv.

Run from esp/wand:   python3 tools/gen_templates.py

This is also the reference implementation of the stroke processing in
main/shape.c (quaternion -> 2D path -> $P), so keep the two in step.
Prints leave-one-out accuracy and sets the reject distance from the data.
"""

import csv, glob, math, os, sys

SHAPES = ["triangle", "zigzag", "circle", "v"]  # order = bns_shape_t values 1..4
N = 32          # points per normalised stroke
TRIM_MS = 100   # ignore button press/release wobble at each end
MIN_POINTS = 10 # shorter strokes (after trimming) are rejected

# ---- stroke -> 2D path (where the wand tip points, in the vertical plane) ----

def tip_direction(w, x, y, z):
    # rotate the sensor X axis (wand tip) into the world frame (Z up)
    return (1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y))

def stroke_path(samples):
    """samples: [(t_ms, qw, qx, qy, qz)] -> [(x right, y up)] in degrees"""
    t0, t1 = samples[0][0], samples[-1][0]
    core = [s for s in samples if t0 + TRIM_MS <= s[0] <= t1 - TRIM_MS] or samples
    pts, yaw0 = [], None
    for _, w, x, y, z in core:
        vx, vy, vz = tip_direction(w, x, y, z)
        yaw = math.degrees(math.atan2(vy, vx))
        pitch = math.degrees(math.asin(max(-1.0, min(1.0, vz))))
        if yaw0 is None:
            yaw0 = yaw
        rel = (yaw - yaw0 + 180.0) % 360.0 - 180.0
        pts.append((-rel, pitch))  # yaw grows to the left; flip so x grows right
    return pts

# ---- $P point-cloud recogniser (Vatavu, Anthony & Wobbrock 2012) ----

def resample(pts):
    length = sum(math.dist(pts[i - 1], pts[i]) for i in range(1, len(pts)))
    interval = length / (N - 1) if length > 0 else 1.0
    out, D, prev = [pts[0]], 0.0, pts[0]
    for cur in pts[1:]:
        d = math.dist(prev, cur)
        while D + d >= interval and d > 0 and len(out) < N:
            t = (interval - D) / d
            prev = (prev[0] + t * (cur[0] - prev[0]), prev[1] + t * (cur[1] - prev[1]))
            out.append(prev)
            d, D = math.dist(prev, cur), 0.0
        D += d
        prev = cur
    while len(out) < N:
        out.append(pts[-1])
    return out[:N]

def normalize(pts):
    pts = resample(pts)
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    s = max(max(xs) - min(xs), max(ys) - min(ys)) or 1.0
    pts = [((x - min(xs)) / s, (y - min(ys)) / s) for x, y in pts]
    cx, cy = sum(p[0] for p in pts) / N, sum(p[1] for p in pts) / N
    return [(x - cx, y - cy) for x, y in pts]

def cloud_distance(a, b, start):
    matched, total, i = [False] * N, 0.0, start
    while True:
        best, idx = float("inf"), -1
        for j in range(N):
            if not matched[j]:
                d = math.dist(a[i], b[j])
                if d < best:
                    best, idx = d, j
        matched[idx] = True
        total += (1 - ((i - start + N) % N) / N) * best
        i = (i + 1) % N
        if i == start:
            return total

def match(a, b):
    step = int(math.sqrt(N))
    return min(min(cloud_distance(a, b, i), cloud_distance(b, a, i)) for i in range(0, N, step))

# ---- load recordings ----

def load_strokes(paths):
    strokes = {}  # (file, stroke) -> (label, [samples])
    for path in paths:
        with open(path) as f:
            rows = [r for r in csv.reader(l for l in f if not l.startswith("#"))][1:]
        for label, stroke, t, qw, qx, qy, qz in rows:
            key = (path, int(stroke))
            lab, samples = strokes.setdefault(key, (label, []))
            if lab != label:
                strokes[key] = ("mixed", samples)  # straddles a shape change: drop
            samples.append((int(t), float(qw), float(qx), float(qy), float(qz)))
    return [(lab, s) for lab, s in strokes.values() if lab in SHAPES]

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    strokes = load_strokes(sorted(glob.glob(os.path.join(root, "captures", "*.csv"))))
    data = []
    for label, samples in strokes:
        path = stroke_path(samples)
        if len(path) >= MIN_POINTS:
            data.append((label, normalize(path)))
    print(f"{len(data)} strokes:", {s: sum(l == s for l, _ in data) for s in SHAPES})

    # leave-one-out: each stroke vs all the others
    correct, right_d = 0, []
    for i, (label, pts) in enumerate(data):
        guess, dist = min(((l, match(pts, t)) for j, (l, t) in enumerate(data) if j != i),
                          key=lambda r: r[1])
        if guess == label:
            correct += 1
            right_d.append(dist)
    right_d.sort()
    reject = round(right_d[-1] * 1.25, 3)  # 25% headroom over the worst correct match
    print(f"leave-one-out accuracy {100 * correct / len(data):.1f}%, "
          f"correct-match distance median {right_d[len(right_d) // 2]:.3f} "
          f"max {right_d[-1]:.3f} -> reject above {reject}")

    out = os.path.join(root, "main", "shape_templates.h")
    with open(out, "w") as f:
        f.write("// Generated by tools/gen_templates.py from captures/*.csv -- do not edit.\n")
        f.write(f"// {len(data)} templates; leave-one-out accuracy {100 * correct / len(data):.1f}%.\n\n")
        f.write("#pragma once\n\n#include \"shape.h\"\n\n")
        f.write(f"#define SHAPE_REJECT_DISTANCE {reject}f\n\n")
        f.write("static const shape_template_t SHAPE_TEMPLATES[] = {\n")
        for label, pts in data:
            coords = ", ".join(f"{{{x:.4f}f, {y:.4f}f}}" for x, y in pts)
            f.write(f"    {{BNS_SHAPE_{label.upper()}, {{{coords}}}}},\n")
        f.write("};\n")
    print("wrote", os.path.relpath(out, root))

if __name__ == "__main__":
    sys.exit(main())
