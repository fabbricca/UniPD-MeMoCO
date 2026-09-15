#!/bin/bash
# Parameter calibration of the tabu search on the calibration instances
# (calib_*.dat, generated with a different seed from the benchmark).
# Every configuration gets the same iteration budget, so all of them do the
# same amount of work regardless of machine load. One parameter is varied at a
# time around the default configuration.
#   scripts/calibrate.sh
# Output: results/calibration.csv with an extra "config" column.
set -e
cd "$(dirname "$0")/.."
OUT=results/calibration.csv
ITER=50000
SEEDS=5
echo "config,$(./tsp_tabu --header)" > "$OUT"
run() {   # run <config name> <extra flags...>
  local name=$1; shift
  for f in instances/calib_*.dat; do
    for s in $(seq 1 $SEEDS); do
      echo -n "$name," >> "$OUT"
      ./tsp_tabu "$f" --seed "$s" --max-iter $ITER --time-limit 1000 "$@" >> "$OUT"
    done
  done
}
# diversification strength (number of double-bridge moves per kick; 0 = none)
run kick0   --kick 0
run kick1   --kick 1
run kick3   --kick 3
run kick6   --kick 6
# how long to wait before a kick (in multiples of n)
run wait0.5 --no-improve-frac 0.5
run wait2   --no-improve-frac 2
run wait5   --no-improve-frac 5
# tabu tenure (in multiples of n)
run ten0.05 --tenure-frac 0.05
run ten0.25 --tenure-frac 0.25
run ten0.5  --tenure-frac 0.5
# initial solution
run random  --init random
echo "done: $OUT"
