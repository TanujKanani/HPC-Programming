#!/bin/bash

echo "=========================================="
echo "  HPC Benchmark Suite — Build Phase"
echo "=========================================="

g++ input_file_maker.cpp -o input_maker.out
g++ -fopenmp main.cpp utils.cpp init.cpp -lm -O3 -o main.out

echo "[OK] Binaries ready."
echo ""

RESULT_FILE="results.csv"
printf "scenario;threads;walltime_interp;walltime_norm;walltime_mover;walltime_denorm;void_count;walltime_alg\n" > "$RESULT_FILE"

scenarios=(
    "A 250 100 900000 10"
    "B 250 100 5000000 10"
    "C 500 200 3600000 10"
    "D 500 200 20000000 10"
    "E 1000 400 14000000 10"
)

thread_list=(1 2 4 8 16)

for entry in "${scenarios[@]}"; do
    read -r tag gx gy npts niter <<< "$entry"

    echo "=========================================="
    printf "  Scenario %-2s | Grid %sx%s | Particles %s\n" "$tag" "$gx" "$gy" "$npts"
    echo "=========================================="

    echo "$gx $gy $npts $niter" | ./input_maker.out > /dev/null

    for t in "${thread_list[@]}"; do
        export OMP_NUM_THREADS=$t

        raw=$(./main.out input.bin)

        w_interp=$(echo "$raw" | grep "Total Interpolation Time" | awk '{print $5}')
        w_norm=$(echo "$raw" | grep "Total Normalization Time" | awk '{print $5}')
        w_mover=$(echo "$raw" | grep "Total Mover Time" | awk '{print $5}')
        w_denorm=$(echo "$raw" | grep "Total Denormalization Time" | awk '{print $5}')
        w_alg=$(echo "$raw" | grep "Total Algorithm Time" | awk '{print $5}')
        n_voids=$(echo "$raw" | grep "Total Number of Voids" | awk '{print $6}')

        printf "    [%2d threads]  alg=%ss  interp=%ss  mover=%ss  voids=%s\n" \
               "$t" "$w_alg" "$w_interp" "$w_mover" "$n_voids"

        printf "%s;%s;%s;%s;%s;%s;%s;%s\n" \
               "$tag" "$t" "$w_interp" "$w_norm" "$w_mover" "$w_denorm" "$n_voids" "$w_alg" >> "$RESULT_FILE"
    done
    echo ""
done

echo "=========================================="
echo "  Finished — results written to $RESULT_FILE"
echo "=========================================="
