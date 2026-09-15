#!/usr/bin/env python3
"""Plot an instance and a tour:  python3 scripts/plot_tour.py <instance> <tour file> <out.pdf> [title]"""
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

inst, tourf, out = sys.argv[1], sys.argv[2], sys.argv[3]
title = sys.argv[4] if len(sys.argv) > 4 else ""
with open(inst) as f:
    n = int(f.readline())
    pts = [tuple(map(float, f.readline().split())) for _ in range(n)]
with open(tourf) as f:
    head = f.readline().split()
    tour = [int(f.readline()) for _ in range(n)]
xs = [pts[v][0] for v in tour] + [pts[tour[0]][0]]
ys = [pts[v][1] for v in tour] + [pts[tour[0]][1]]
plt.figure(figsize=(4.2, 4.2))
plt.plot(xs, ys, "-", color="tab:blue", lw=0.9)
plt.plot([p[0] for p in pts], [p[1] for p in pts], "o", color="black", ms=2.5)
plt.plot(pts[tour[0]][0], pts[tour[0]][1], "s", color="tab:red", ms=5)
plt.xlim(-2, 102); plt.ylim(-2, 102); plt.gca().set_aspect("equal")
plt.xlabel("x [mm]"); plt.ylabel("y [mm]")
if title: plt.title("%s (length %.1f)" % (title, float(head[1])), fontsize=10)
plt.tight_layout(); plt.savefig(out)
