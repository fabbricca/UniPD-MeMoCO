#!/usr/bin/env python3
"""Summarise the CSV results into the tables of the report (LaTeX + text).

  python3 scripts/analyze.py            (run from the project root)
Writes results/tables/*.tex and prints a text summary.
"""
import glob, os
import pandas as pd

os.makedirs("results/tables", exist_ok=True)
BUDGETS = [0.1, 1, 10, 60]
SIZES = list(range(10, 101, 10))

def budget_name(t):
    return ("%g" % t)

# ---------------------------------------------------------------- Part I
p1 = {}
for t in BUDGETS:
    f = "results/assignment_1_t%s.csv" % budget_name(t)
    if os.path.exists(f):
        d = pd.read_csv(f)
        d["budget"] = t
        p1[t] = d
allp1 = pd.concat(p1.values(), ignore_index=True)

# best known value per instance: best CPLEX incumbent over all budgets
# (updated below with the tabu search results)
best_known = allp1.dropna(subset=["obj"]).groupby("instance")["obj"].min().to_dict()
opt_value = allp1[allp1.optimal == 1].groupby("instance")["obj"].min().to_dict()

# Table: for every budget and size, instances solved to optimality (out of 5),
# average solve time of the solved ones, average gap of the unsolved ones,
# average number of B&B nodes.
rows = []
for t in BUDGETS:
    if t not in p1: continue
    d = p1[t]
    for n in SIZES:
        s = d[d.n == n]
        solved = s[s.optimal == 1]
        unsolved = s[s.optimal == 0]
        rows.append({
            "budget": t, "n": n, "solved": len(solved), "with_sol": int(s.has_sol.sum()),
            "time_solved": solved.solve_s.mean() if len(solved) else float("nan"),
            "time_max": solved.solve_s.max() if len(solved) else float("nan"),
            "gap_unsolved": unsolved.gap.mean() * 100 if len(unsolved) and unsolved.gap.notna().any() else float("nan"),
            "nodes": s.nodes.mean(),
        })
tab1 = pd.DataFrame(rows)
tab1.to_csv("results/tables/assignment_1_by_size.csv", index=False)

with open("results/tables/assignment_1_by_size.tex", "w") as f:
    f.write("\\begin{tabular}{r" + "ccc" * len(p1) + "}\n\\toprule\n")
    f.write("& " + " & ".join("\\multicolumn{3}{c}{%g s}" % t for t in p1) + " \\\\\n")
    f.write(" ".join("\\cmidrule(lr){%d-%d}" % (2 + 3 * i, 4 + 3 * i) for i in range(len(p1))) + "\n")
    f.write("$n$ " + "& solved & time (s) & gap (\\%)" * len(p1) + " \\\\\n\\midrule\n")
    for n in SIZES:
        cells = []
        for t in p1:
            r = tab1[(tab1.budget == t) & (tab1.n == n)].iloc[0]
            tm = "--" if pd.isna(r.time_solved) else "%.2f" % r.time_solved
            gp = "--" if pd.isna(r.gap_unsolved) else "%.1f" % r.gap_unsolved
            if r.with_sol < 5 and r.solved < 5:
                gp += "$^{%d}$" % (5 - r.with_sol)
            cells += ["%d/5" % r.solved, tm, gp]
        f.write("%d & " % n + " & ".join(cells) + " \\\\\n")
    f.write("\\bottomrule\n\\end{tabular}\n")

# Threshold: largest n such that all 5 instances are solved within the budget,
# and largest n such that at least one is.
thr = []
for t in p1:
    d = tab1[tab1.budget == t]
    all5 = [r.n for r in d.itertuples() if r.solved == 5]
    any1 = [r.n for r in d.itertuples() if r.solved >= 1]
    anysol = [r.n for r in d.itertuples() if r.with_sol == 5]
    thr.append({"budget": t, "all_solved": max(all5) if all5 else 0,
                "some_solved": max(any1) if any1 else 0,
                "feasible_all": max(anysol) if anysol else 0})
thr = pd.DataFrame(thr)
thr.to_csv("results/tables/assignment_1_thresholds.csv", index=False)

# ---------------------------------------------------------------- Part II
p2 = {}
for f in sorted(glob.glob("results/assignment_2_t*.csv")):
    t = float(f.split("_t")[1][:-4])
    d = pd.read_csv(f)
    d["budget"] = t
    p2[t] = d
if p2:
    allp2 = pd.concat(p2.values(), ignore_index=True)
    for inst, v in allp2.groupby("instance")["best_cost"].min().items():
        best_known[inst] = min(best_known.get(inst, float("inf")), v)
    allp2["ref"] = allp2.instance.map(best_known)
    allp2["gap"] = (allp2.best_cost / allp2.ref - 1) * 100
    allp2["is_opt_ref"] = allp2.instance.map(lambda i: i in opt_value)
    rows = []
    for t in sorted(p2):
        d = allp2[allp2.budget == t]
        for n in SIZES:
            s = d[d.n == n]
            per_inst = s.groupby("instance").gap
            rows.append({"budget": t, "n": n, "runs": len(s),
                         "gap_mean": s.gap.mean(), "gap_std": s.gap.std(),
                         "gap_min": per_inst.min().mean(), "gap_max": per_inst.max().mean(),
                         "hits": (s.gap < 1e-6).mean() * 100,
                         "iters": s.iterations.mean(), "time_to_best": s.time_to_best.mean()})
    tab2 = pd.DataFrame(rows)
    tab2.to_csv("results/tables/assignment_2_by_size.csv", index=False)
    with open("results/tables/assignment_2_by_size.tex", "w") as f:
        bs = sorted(p2)
        f.write("\\begin{tabular}{r" + "cccc" * len(bs) + "}\n\\toprule\n")
        f.write("& " + " & ".join("\\multicolumn{4}{c}{%g s}" % t for t in bs) + " \\\\\n")
        f.write(" ".join("\\cmidrule(lr){%d-%d}" % (2 + 4 * i, 5 + 4 * i) for i in range(len(bs))) + "\n")
        f.write("$n$ " + "& mean & std & worst & hits" * len(bs) + " \\\\\n\\midrule\n")
        for n in SIZES:
            cells = []
            for t in bs:
                r = tab2[(tab2.budget == t) & (tab2.n == n)].iloc[0]
                cells += ["%.2f" % r.gap_mean, "%.2f" % r.gap_std, "%.2f" % r.gap_max, "%.0f\\%%" % r.hits]
            f.write("%d & " % n + " & ".join(cells) + " \\\\\n")
        f.write("\\bottomrule\n\\end{tabular}\n")

    # head-to-head at equal budget: CPLEX incumbent vs tabu mean, per size
    rows = []
    for t in [0.1, 1, 10]:
        if t not in p1 or t not in p2: continue
        c = p1[t]; h = allp2[allp2.budget == t]
        for n in SIZES:
            cs = c[c.n == n]; hs = h[h.n == n]
            cgap = ((cs.obj / cs.instance.map(best_known)) - 1) * 100
            rows.append({"budget": t, "n": n, "cplex_feasible": int(cs.has_sol.sum()),
                         "cplex_optimal": int(cs.optimal.sum()),
                         "cplex_gap": cgap.mean(), "tabu_gap": hs.gap.mean(),
                         "tabu_better": int((hs.groupby("instance").gap.mean() < cgap.set_axis(cs.instance) - 1e-6).sum())})
    tab3 = pd.DataFrame(rows)
    tab3.to_csv("results/tables/head_to_head.csv", index=False)
    with open("results/tables/head_to_head.tex", "w") as f:
        bs = [t for t in [0.1, 1, 10] if t in p1 and t in p2]
        f.write("\\begin{tabular}{r" + "ccc" * len(bs) + "}\n\\toprule\n")
        f.write("& " + " & ".join("\\multicolumn{3}{c}{%g s}" % t for t in bs) + " \\\\\n")
        f.write(" ".join("\\cmidrule(lr){%d-%d}" % (2 + 3 * i, 4 + 3 * i) for i in range(len(bs))) + "\n")
        f.write("$n$ " + "& CPLEX opt. & CPLEX gap & TS gap" * len(bs) + " \\\\\n\\midrule\n")
        for n in SIZES:
            cells = []
            for t in bs:
                r = tab3[(tab3.budget == t) & (tab3.n == n)].iloc[0]
                cg = "--" if pd.isna(r.cplex_gap) else "%.1f" % r.cplex_gap
                if r.cplex_feasible < 5: cg += "$^{%d}$" % (5 - r.cplex_feasible)
                cells += ["%d/5" % r.cplex_optimal, cg, "%.2f" % r.tabu_gap]
            f.write("%d & " % n + " & ".join(cells) + " \\\\\n")
        f.write("\\bottomrule\n\\end{tabular}\n")

# ---------------------------------------------------------------- calibration
if os.path.exists("results/calibration.csv"):
    cal = pd.read_csv("results/calibration.csv")
    ref = {}
    if os.path.exists("results/calib_reference.csv"):
        r = pd.read_csv("results/calib_reference.csv").dropna(subset=["obj"])
        ref = r.set_index("instance").obj.to_dict()
    for inst, v in cal.groupby("instance").best_cost.min().items():
        ref[inst] = min(ref.get(inst, float("inf")), v)
    cal["gap"] = (cal.best_cost / cal.instance.map(ref) - 1) * 100
    g = cal.groupby("config").agg(gap=("gap", "mean"), std=("gap", "std"), worst=("gap", "max"),
                                  iters=("iterations", "mean"), best_iter=("best_iter", "mean"),
                                  time=("total_s", "mean"))
    order = ["kick0", "kick1", "kick3", "kick6", "wait0.5", "wait2", "wait5",
             "ten0.05", "ten0.25", "ten0.5", "random"]
    g = g.reindex([o for o in order if o in g.index])
    g.to_csv("results/tables/calibration.csv")
    desc = {"kick0": "no kicks (plain tabu search)", "kick1": "1 double bridge", "kick3": "3 double bridges",
            "kick6": "6 double bridges (default)", "wait0.5": "kick after $0.5n$ idle it.",
            "wait2": "kick after $2n$ idle it.", "wait5": "kick after $5n$ idle it.",
            "ten0.05": "tenure $0.05n$", "ten0.25": "tenure $0.25n$", "ten0.5": "tenure $0.5n$",
            "random": "random initial tour"}
    with open("results/tables/calibration.tex", "w") as f:
        f.write("\\begin{tabular}{llrrrrr}\n\\toprule\nconfig. & description & mean gap & std & worst & it. to best & time (s) \\\\\n\\midrule\n")
        for c, r in g.iterrows():
            f.write("%s & %s & %.3f & %.3f & %.2f & %.0f & %.2f \\\\\n" % (c.replace("_", "\\_"), desc.get(c, ""), r.gap, r["std"], r.worst, r.best_iter, r.time))
        f.write("\\bottomrule\n\\end{tabular}\n")
    print("\n== calibration (gap % to best known)\n", g.round(3))

print("== assignment_1 by size\n", tab1.round(3).to_string(index=False))
print("\n== thresholds\n", thr.to_string(index=False))
if p2:
    print("\n== assignment_2 by size\n", tab2.round(3).to_string(index=False))
    print("\n== head to head\n", tab3.round(3).to_string(index=False))

# ---------------------------------------------------------------- per-instance tables
def fmt_cell(r):
    if r.optimal == 1: return "%.2f" % r.solve_s
    if r.has_sol == 1 and not pd.isna(r.gap): return "(%.0f\\%%)" % (r.gap * 100)
    return "--"
with open("results/tables/assignment_1_instances.tex", "w") as f:
    f.write("\\begin{longtable}{lrrrrrrr}\n")
    f.write("\\caption{Part I, every instance: best known value (optimum when proved), solution time in seconds when solved within the budget, otherwise the relative gap in parentheses; branch-and-bound nodes of the 60 s run.}\\label{tab:p1inst}\\\\\n")
    f.write("\\toprule\ninstance & $n$ & best known & 0.1 s & 1 s & 10 s & 60 s & nodes \\\\\n\\midrule\n\\endfirsthead\n")
    f.write("\\toprule\ninstance & $n$ & best known & 0.1 s & 1 s & 10 s & 60 s & nodes \\\\\n\\midrule\n\\endhead\n")
    for n in SIZES:
        for k in range(1, 6):
            inst = "tsp_n%d_%d" % (n, k)
            cells = []
            for t in BUDGETS:
                r = p1[t][p1[t].instance == inst].iloc[0]
                cells.append(fmt_cell(r))
            r60 = p1[60][p1[60].instance == inst].iloc[0]
            bk = best_known.get(inst, float("nan"))
            star = "" if inst in opt_value else "$^*$"
            f.write("%s & %d & %.1f%s & %s & %d \\\\\n" % (inst.replace("_", "\\_"), n, bk, star, " & ".join(cells), r60.nodes))
    f.write("\\bottomrule\n\\end{longtable}\n")

if p2:
    with open("results/tables/assignment_2_instances.tex", "w") as f:
        f.write("\\begin{longtable}{lrrrrrrrr}\n")
        f.write("\\caption{Part II, every instance: gap (\\%) to the best known value of the mean and of the worst run, for each budget, and average time (s) at which the best tour of the 1 s runs was found.}\\label{tab:p2inst}\\\\\n")
        f.write("\\toprule\n & & \\multicolumn{2}{c}{0.1 s} & \\multicolumn{3}{c}{1 s} & \\multicolumn{2}{c}{10 s} \\\\\n\\cmidrule(lr){3-4}\\cmidrule(lr){5-7}\\cmidrule(lr){8-9}\ninstance & best known & mean & worst & mean & worst & $t_{best}$ & mean & worst \\\\\n\\midrule\n\\endfirsthead\n")
        f.write("\\toprule\ninstance & best known & mean & worst & mean & worst & $t_{best}$ & mean & worst \\\\\n\\midrule\n\\endhead\n")
        for n in SIZES:
            for k in range(1, 6):
                inst = "tsp_n%d_%d" % (n, k)
                cells = []
                for t in [0.1, 1, 10]:
                    s = allp2[(allp2.budget == t) & (allp2.instance == inst)]
                    cells += ["%.2f" % s.gap.mean(), "%.2f" % s.gap.max()]
                    if t == 1: cells.append("%.3f" % s.time_to_best.mean())
                f.write("%s & %.1f & %s \\\\\n" % (inst.replace("_", "\\_"), best_known[inst], " & ".join(cells)))
        f.write("\\bottomrule\n\\end{longtable}\n")

    # figure: mean gap vs n for each budget, and CPLEX gap
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    plt.figure(figsize=(5.4, 3.4))
    for t, mk in [(0.1, "o-"), (1, "s-"), (10, "^-")]:
        d = tab2[tab2.budget == t]
        plt.plot(d.n, d.gap_mean, mk, ms=4, label="tabu search, %g s" % t)
    plt.xlabel("number of holes n"); plt.ylabel("mean gap to best known [%]")
    plt.xticks(SIZES); plt.grid(alpha=0.3); plt.legend(fontsize=8); plt.tight_layout()
    plt.savefig("report/figures/tabu_gap.pdf")
