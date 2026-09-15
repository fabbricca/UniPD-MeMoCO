# [UniPD] MeMoCO - lab assignments

Riccardo Fabbian, 2140711. *Methods and Models for Combinatorial Optimization*,
University of Padova, A.Y. 2025/26.

**Problem.** A drilling machine must make `n` holes on a board and return to the first
one; drilling time is constant, so the sequence that minimises the total time is a
minimum-cost Hamiltonian cycle on the holes: a Travelling Salesman Problem.

**Assignment 1 (exact).** The compact single-commodity flow formulation of Gavish and
Graves, built with the CPLEX Callable Library (bulk `CPXnewcols`/`CPXaddrows`, sparse row
format, one thread, MIP gap 1e-6, tour extraction and check). Tested on 50 seeded
instances (10-100 holes, 5 per size) with time limits of 0.1, 1, 10 and 60 s, one
campaign per limit. Result: all instances of a size solved to proven optimality up to
10 / 20 / 40 / 60 holes respectively.

**Assignment 2 (heuristic).** A tabu search: path representation, nearest-neighbour
start, 2-opt neighbourhood with O(1) incremental evaluation, arc-based tabu memory with
O(1) check, aspiration on the global best, double-bridge kicks as diversification.
Parameters calibrated on 8 separate instances with an iteration budget. Result: the
optimum in every run up to 50 holes in 0.1 s and up to 90 holes in 1 s; 0.5 % mean gap at
100 holes with 0.1 s. Never beaten by the CPLEX incumbent at equal time.

The 21-page report (`report/main.pdf`) describes the model, the implementation, the
design choices, the calibration and the results of both parts. Every number in it is
reproducible from the scripts and the fixed seeds. Code tested on the LabTA machines
(g++ 12, CPLEX 22.1.1).

## Contents

```
common/     instance.h (instance file + cost matrix), timer.h
assignment_1/  main.cpp, tsp_model.h/.cpp, cpxmacro.h
assignment_2/  main.cpp, tabu_search.h/.cpp
instances/  50 benchmark instances tsp_n<size>_<k>.dat and 8 calibration instances calib_*.dat
            (all regenerated identically by scripts/gen_instances.py, fixed seeds)
scripts/    gen_instances.py, run_assignment_1.sh, run_assignment_2.sh, calibrate.sh, analyze.py
results/    CSV output of every run, results/tables/ (report tables)
report/     main.tex / main.pdf
```

## Build

```
make                       # lab machines (/opt/ibm/ILOG/CPLEX_Studio2211) and the student
                           # server ssh.studenti.math.unipd.it (software share under /mnt)
make CPX_BASE=/path/to/CPLEX_Studio   # elsewhere
make tsp_tabu              # Part II only, no CPLEX needed
```

Requires g++ (C++11) and CPLEX 22.1 or later. Tested with g++ 12.2 / CPLEX 22.1.1
on the student server and with g++ 16 / CPLEX 22.2 on the development machine.

## Run

```
./tsp_cplex instances/tsp_n30_1.dat --time-limit 10            # Part I
./tsp_tabu  instances/tsp_n30_1.dat --time-limit 1 --seed 1    # Part II
```

Both print one CSV line (`--header` prints the column names, `--tour FILE`
writes the tour). Options of `tsp_cplex`: `--time-limit S` (60), `--threads K`
(1), `--gap G` (1e-6), `--verbose`. Options of `tsp_tabu`: `--seed S`,
`--init nn|random`, `--tenure-frac F` (0.125), `--no-improve-frac F` (0.5),
`--kick K` (6), `--max-iter N`, `--time-limit S` (10).

Instance file format: first line `n`, then one line `x y` per hole
(millimetres). Costs are Euclidean distances, computed when the file is read.

## Reproduce the experiments

The generator is deterministic (fixed seeds), so the complete benchmark used in the
report can always be recreated:

```
python3 scripts/gen_instances.py instances      # regenerate all 58 instances (fixed seeds)
scripts/run_assignment_1.sh 0.1; scripts/run_assignment_1.sh 1; scripts/run_assignment_1.sh 10; scripts/run_assignment_1.sh 60
scripts/calibrate.sh                            # Part II calibration (iteration budget)
scripts/run_assignment_2.sh 0.1 10; scripts/run_assignment_2.sh 1 10; scripts/run_assignment_2.sh 10 5
python3 scripts/analyze.py                      # tables in results/tables/ (needs pandas + matplotlib;
                                                # not installed on the lab machines, tables are included)
```
