#!/bin/bash
# Run the tabu search on every benchmark instance with one time limit and
# several seeds.
#   scripts/run_assignment_2.sh <time limit in seconds> <number of seeds>
# Output: results/assignment_2_t<limit>.csv
set -e
cd "$(dirname "$0")/.."
T=${1:-1}
SEEDS=${2:-10}
OUT=results/assignment_2_t${T}.csv
./tsp_tabu --header > "$OUT"
for n in 10 20 30 40 50 60 70 80 90 100; do
  for k in 1 2 3 4 5; do
    [ -f instances/tsp_n${n}_${k}.dat ] || continue   # instance not present (only a few are shipped)
    for s in $(seq 1 "$SEEDS"); do
      ./tsp_tabu instances/tsp_n${n}_${k}.dat --seed "$s" --time-limit "$T" >> "$OUT"
    done
  done
done
echo "done: $OUT"
