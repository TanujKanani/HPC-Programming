#!/bin/bash
# =============================================================================
# HPC Assignment 06 - Benchmark Runner
# Generates all 5 input configs
# Thread=1  -> serial binary (compiled WITHOUT -fopenmp, all #pragma ignored)
# Thread=2+ -> parallel binary (compiled WITH -fopenmp)
# Saves results to results.csv
# =============================================================================

set -e

CSV="results.csv"
echo "Config,NX,NY,Points,Maxiter,Threads,TotalTime_sec,AvgIterTime_sec" > "$CSV"

# ── Compile ──
echo "=== Compiling ==="

# Serial build: NO -fopenmp, all #pragma omp lines are ignored by compiler
g++ -O2 -o main_serial main.cpp init.cpp utils.cpp
echo "  main_serial built (no OpenMP, #pragma ignored)"

# Parallel build: WITH -fopenmp
g++ -O2 -fopenmp -o main_parallel main.cpp init.cpp utils.cpp
echo "  main_parallel built (OpenMP enabled)"

# Input file maker
g++ -O2 -o input_file_maker input_file_maker.cpp
echo "  input_file_maker built"

# ── Configurations: NX NY Points Maxiter ──
NX_ARR=(250    250    500    500     1000)
NY_ARR=(100    100    200    200     400)
PT_ARR=(900000 5000000 3600000 20000000 14000000)
IT_ARR=(10     10     10     10      10)
LABELS=("A"    "B"    "C"    "D"     "E")

THREADS=(1 2 4 8 16)

# ── Generate input files ──
echo ""
echo "=== Generating input files ==="
for i in "${!LABELS[@]}"; do
    LABEL="${LABELS[$i]}"
    INPUT_FILE="input_${LABEL}.bin"

    if [ -f "$INPUT_FILE" ]; then
        echo "  Config $LABEL: $INPUT_FILE already exists, skipping"
    else
        echo "  Config $LABEL: NX=${NX_ARR[$i]} NY=${NY_ARR[$i]} Points=${PT_ARR[$i]} Maxiter=${IT_ARR[$i]}"
        printf "%d %d\n%d\n%d\n" "${NX_ARR[$i]}" "${NY_ARR[$i]}" "${PT_ARR[$i]}" "${IT_ARR[$i]}" | ./input_file_maker
        mv input.bin "$INPUT_FILE"
        echo "    -> $INPUT_FILE generated"
    fi
done

# ── Run benchmarks ──
echo ""
echo "=== Running benchmarks ==="
echo "───────────────────────────────────────────────────────────────────────────────"
printf "%-8s %6s %6s %12s %6s %8s %6s %14s %14s\n" \
       "Config" "NX" "NY" "Points" "Iter" "Threads" "Build" "Total(s)" "Avg/Iter(s)"
echo "───────────────────────────────────────────────────────────────────────────────"

for i in "${!LABELS[@]}"; do
    LABEL="${LABELS[$i]}"
    NX="${NX_ARR[$i]}"
    NY="${NY_ARR[$i]}"
    PTS="${PT_ARR[$i]}"
    ITER="${IT_ARR[$i]}"
    INPUT_FILE="input_${LABEL}.bin"

    for T in "${THREADS[@]}"; do
        export OMP_NUM_THREADS=$T

        if [ "$T" -eq 1 ]; then
            # TRUE serial: compiled without -fopenmp, zero OpenMP overhead
            EXEC="./main_serial"
            BUILD="serial"
        else
            EXEC="./main_parallel"
            BUILD="omp"
        fi

        # Run and capture output
        OUTPUT=$($EXEC "$INPUT_FILE" 2>&1)

        # Parse times from main's printf output
        TOTAL=$(echo "$OUTPUT" | grep "Total interpolation time" | awk -F'= ' '{print $2}' | awk '{print $1}')
        AVG=$(echo "$OUTPUT"   | grep "Average time per iteration" | awk -F'= ' '{print $2}' | awk '{print $1}')

        # Print to terminal
        printf "%-8s %6d %6d %12d %6d %8d %6s %14s %14s\n" \
               "$LABEL" "$NX" "$NY" "$PTS" "$ITER" "$T" "$BUILD" "$TOTAL" "$AVG"

        # Write to CSV
        echo "${LABEL},${NX},${NY},${PTS},${ITER},${T},${TOTAL},${AVG}" >> "$CSV"
    done

    echo "───────────────────────────────────────────────────────────────────────────────"
done

echo ""
echo "=== Done! Results saved to $CSV ==="
echo ""
echo "Speedup = T_serial / T_parallel (use Thread=1 row as T_serial for each config)"
echo "Efficiency = Speedup / Threads"