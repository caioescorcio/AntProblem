#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

make all

RESULTS_DIR="results/sweeps_render_food10"
mkdir -p "$RESULTS_DIR"

ANTS_LIST=(5000)
POST_FIRST_FOOD_ITERATIONS="${POST_FIRST_FOOD_ITERATIONS:-10}"

for ants in "${ANTS_LIST[@]}"; do
  iter_csv="$RESULTS_DIR/iter_threads1_ants${ants}.csv"
  summary_csv="$RESULTS_DIR/summary_threads1_ants${ants}.csv"

  echo "Running vectorized headless sweep: threads=1, ants=${ants}"
  ./ant_simu.exe \
    --headless \
    --nb-ants "$ants" \
    --post-first-food-iterations "$POST_FIRST_FOOD_ITERATIONS" \
    --timing-csv "$iter_csv" \
    --summary-csv "$summary_csv"
done

echo "Done. Results saved in: $RESULTS_DIR"
