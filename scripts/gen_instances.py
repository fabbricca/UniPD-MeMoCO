#!/usr/bin/env python3
"""Generate the benchmark instances.

Holes are placed uniformly at random on a 100 x 100 mm board, with a minimum
distance of 1 mm between any two holes (a drill cannot make two overlapping
holes). Sizes 10, 20, ..., 100, five instances per size, fixed seed so that the
benchmark is reproducible. A separate, smaller set with a different seed is
generated for the calibration of the tabu search (Part II), so that parameters
are not tuned on the instances used to report results.
"""
import os, random, sys

BOARD = 100.0      # mm
MIN_DIST = 1.0     # mm

def gen(n, rng):
    pts = []
    while len(pts) < n:
        px, py = rng.uniform(0, BOARD), rng.uniform(0, BOARD)
        if all((px - qx) ** 2 + (py - qy) ** 2 >= MIN_DIST ** 2 for qx, qy in pts):
            pts.append((round(px, 2), round(py, 2)))
    return pts

def write(path, pts):
    with open(path, "w") as f:
        f.write("%d\n" % len(pts))
        for px, py in pts:
            f.write("%.2f %.2f\n" % (px, py))

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "instances"
    os.makedirs(out, exist_ok=True)
    # benchmark set: seed 2026
    rng = random.Random(2026)
    for n in range(10, 101, 10):
        for k in range(1, 6):
            write(os.path.join(out, "tsp_n%d_%d.dat" % (n, k)), gen(n, rng))
    # calibration set: seed 7, sizes 30/50/70/100, two instances each
    rng = random.Random(7)
    for n in (30, 50, 70, 100):
        for k in (1, 2):
            write(os.path.join(out, "calib_n%d_%d.dat" % (n, k)), gen(n, rng))

if __name__ == "__main__":
    main()
