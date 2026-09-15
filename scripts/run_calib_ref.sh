#!/bin/bash
# Reference values for the calibration instances: CPLEX with 60 s.
set -e
cd "$(dirname "$0")/.."
OUT=results/calib_reference.csv
./tsp_cplex --header > "$OUT"
for f in instances/calib_*.dat; do ./tsp_cplex "$f" --time-limit 60 >> "$OUT" || true; done
echo "done: $OUT"
