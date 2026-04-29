#!/usr/bin/env bash
# =====================================================================
#  generate_inputs.sh
#  Creates the 5 binary input files required by the assignment.
#  Run this ONCE before any scaling experiments.
#
#  Usage:
#    chmod +x generate_inputs.sh
#    ./generate_inputs.sh
# =====================================================================
set -euo pipefail

MAKER=./input_maker

if [ ! -x "$MAKER" ]; then
    echo "[ERROR] '$MAKER' not found – run 'make' first."
    exit 1
fi

# Helper: generate one input file and rename it.
# Args: NX  NY  NUM_POINTS  MAXITER  OUTPUT_NAME
gen() {
    local NX=$1 NY=$2 NP=$3 ITER=$4 OUT=$5
    printf "%d %d\n%d\n%d\n" "$NX" "$NY" "$NP" "$ITER" | "$MAKER" > /dev/null
    mv input.bin "$OUT"
    echo "  [OK] $OUT  (NX=$NX NY=$NY particles=$NP iters=$ITER)"
}

echo "=== Generating input files ==="
gen  250  100    900000 10  input_a_250x100_900k.bin
gen  250  100   5000000 10  input_b_250x100_5m.bin
gen  500  200   3600000 10  input_c_500x200_3p6m.bin
gen  500  200  20000000 10  input_d_500x200_20m.bin
gen 1000  400  14000000 10  input_e_1000x400_14m.bin
echo "=== Done. Five input files created. ==="
