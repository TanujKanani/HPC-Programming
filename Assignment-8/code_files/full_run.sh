#!/usr/bin/env bash
# =====================================================================
#  full_run.sh  —  Compile + run all 5 configs one at a time
#  (deletes each .bin before generating the next to save quota)
# =====================================================================
set -eu

echo "=========================================="
echo " Step 1: Compiling..."
echo "=========================================="

/usr/mpi/gcc/openmpi-1.8.8/bin/mpicc -O3 -fopenmp -o mpi main.cpp init.cpp utils.cpp -lm
g++ -O2 -std=c++11 -o input_maker input_file_maker.cpp

echo " Compilation done."
echo ""

# Helper: generate bin, run scaling for that one config, then delete bin
run_config() {
    local NX=$1
    local NY=$2
    local POINTS=$3
    local LABEL=$4
    local BINFILE=$5

    echo "=========================================="
    echo " Config $LABEL  ($NX x $NY, $POINTS pts)"
    echo "=========================================="

    # Generate input
    printf "%d %d\n%d\n10\n" "$NX" "$NY" "$POINTS" | ./input_maker > /dev/null
    mv input.bin "$BINFILE"
    echo " Generated: $BINFILE"

    # Run scaling (script skips files that don't exist, so only this one runs)
    bash run_scaling.sh

    # Delete to free quota
    rm -f "$BINFILE"
    echo " Deleted: $BINFILE"
    echo ""
}

# Config A: Nx=250, Ny=100, 0.9M points
run_config 250 100 900000   "A" "input_a_250x100_900k.bin"

# Config B: Nx=250, Ny=100, 5M points
run_config 250 100 5000000  "B" "input_b_250x100_5m.bin"

# Config C: Nx=500, Ny=200, 3.6M points
run_config 500 200 3600000  "C" "input_c_500x200_3p6m.bin"

# Config D: Nx=500, Ny=200, 20M points
run_config 500 200 20000000 "D" "input_d_500x200_20m.bin"

# Config E: Nx=1000, Ny=400, 14M points
run_config 1000 400 14000000 "E" "input_e_1000x400_14m.bin"

echo "=========================================="
echo " All configs done. Results CSVs:"
ls results_*.csv
echo "=========================================="
# Merge all CSVs into one
OUTPUT="results_all.csv"
echo "config,cores,ranks,threads_per_rank,wall_time_s,interp_time_s,mover_time_s" > "$OUTPUT"
for CSV in results_a_250x100_900k.csv results_b_250x100_5m.csv results_c_500x200_3p6m.csv results_d_500x200_20m.csv results_e_1000x400_14m.csv; do
    [ ! -f "$CSV" ] && continue
    CONFIG=$(echo "$CSV" | sed 's/results_//;s/\.csv//')
    tail -n +2 "$CSV" | while IFS= read -r line; do
        echo "${CONFIG},${line}"
    done
done >> "$OUTPUT"