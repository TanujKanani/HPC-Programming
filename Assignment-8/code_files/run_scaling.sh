#!/usr/bin/env bash
# =====================================================================
#  run_scaling.sh
# =====================================================================
set -eu

BINARY=./mpi
HOSTFILE=./source.txt

if [ ! -x "$BINARY" ]; then
    echo "[ERROR] '$BINARY' not found – run 'make' first."; exit 1
fi

declare -A CONFIGS=(
    ["input_a_250x100_900k.bin"]="a_250x100_900k"
    ["input_b_250x100_5m.bin"]="b_250x100_5m"
    ["input_c_500x200_3p6m.bin"]="c_500x200_3p6m"
    ["input_d_500x200_20m.bin"]="d_500x200_20m"
    ["input_e_1000x400_14m.bin"]="e_1000x400_14m"
)

CORE_COUNTS=(2 4 8 16 32 64)

export OMP_PLACES=cores
export OMP_PROC_BIND=close
export OMP_WAIT_POLICY=active

get_ranks_threads() {
    local cores=$1
    if   [ "$cores" -le 32 ]; then
        echo "1 $cores"
    else
        local ranks=$(( cores / 16 ))
        [ "$ranks" -gt 4 ] && ranks=4
        local threads=$(( cores / ranks ))
        echo "$ranks $threads"
    fi
}

echo "=============================================="
echo " Assignment 8 – Scaling Study"
echo " Started : $(date)"
echo "=============================================="

for INPUT in "${!CONFIGS[@]}"; do
    LABEL="${CONFIGS[$INPUT]}"
    CSV_FILE="results_${LABEL}.csv"

    if [ ! -f "$INPUT" ]; then
        continue
    fi

    echo "cores,ranks,threads_per_rank,wall_time_s,interp_time_s,mover_time_s" > "$CSV_FILE"
    echo ""
    echo "--- Config: $LABEL ---"

    for CORES in "${CORE_COUNTS[@]}"; do
        read -r RANKS THREADS <<< "$(get_ranks_threads "$CORES")"
        export OMP_NUM_THREADS=$THREADS

        echo -n "  cores=$CORES  (ranks=$RANKS × threads=$THREADS) ... "

    
        RESULT=$(/usr/mpi/gcc/openmpi-1.8.8/bin/mpirun -np "$RANKS" \
                        --bind-to none \
                        --hostfile "$HOSTFILE" \
                        env OMP_NUM_THREADS="$THREADS" OMP_PLACES=cores OMP_PROC_BIND=close \
                        "$BINARY" "$INPUT" 2>/dev/null \
                 | grep "^CSV," | sed 's/^CSV,//')

        if [ -z "$RESULT" ]; then
            echo "FAILED"
        else
            echo "$CORES,$RANKS,$THREADS,$RESULT" >> "$CSV_FILE"
            WALL_TIME=$(echo "$RESULT" | cut -d',' -f1)
            printf "%.4f s\n" "$WALL_TIME"
        fi
    done
done
echo "=============================================="