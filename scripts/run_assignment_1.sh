#!/bin/bash
# Run the exact method on every benchmark instance with one time limit.
#   scripts/run_assignment_1.sh <time limit in seconds>
# Output: results/assignment_1_t<limit>.csv (one line per instance).
set -e
cd "$(dirname "$0")/.."
T=${1:-60}
mkdir -p results/tours
OUT=results/assignment_1_t${T}.csv
./tsp_cplex --header > "$OUT"
for n in 10 20 30 40 50 60 70 80 90 100; do
  for k in 1 2 3 4 5; do
    [ -f instances/tsp_n${n}_${k}.dat ] || continue   # instance not present (only a few are shipped)
    ./tsp_cplex instances/tsp_n${n}_${k}.dat --time-limit "$T" --tour results/tours/cplex_n${n}_${k}_t${T}.txt >> "$OUT" || true
  done
done
echo "done: $OUT"
