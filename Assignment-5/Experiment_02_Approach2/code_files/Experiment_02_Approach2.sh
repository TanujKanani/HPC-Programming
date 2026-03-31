#!/bin/bash
OUTPUT_CSV="results_exp2_immediate.csv"
echo "Grid_Nx,Grid_Ny,Threads,Approach,Interp_Time_s,Mover_Time_s,Total_Time_s" > $OUTPUT_CSV

GRIDS=( "250 100" "500 200" "1000 400" )
THREADS=( 1 2 4 8 16 )

echo "Compiling Exp 2 - Immediate OpenMP..."
g++ -O3 -march=native -fopenmp exp2_immediate.cpp utils.cpp init.cpp -o run_sim

for grid in "${GRIDS[@]}"; do
    read NX NY <<< "$grid"
    for t in "${THREADS[@]}"; do
        ./run_sim $NX $NY $t > temp.txt
        
        base_line=$(grep "Baseline" temp.txt)
        b_interp=$(echo "$base_line" | awk -F'|' '{print $2}')
        b_mover=$(echo "$base_line" | awk -F'|' '{print $3}')
        b_tot=$(echo "$base_line" | awk -F'|' '{print $4}')
        echo "$NX,$NY,$t,Assign04_Baseline,$b_interp,$b_mover,$b_tot" >> $OUTPUT_CSV

        imm_line=$(grep "Immediate" temp.txt)
        i_interp=$(echo "$imm_line" | awk -F'|' '{print $2}')
        i_mover=$(echo "$imm_line" | awk -F'|' '{print $3}')
        i_tot=$(echo "$imm_line" | awk -F'|' '{print $4}')
        echo "$NX,$NY,$t,Immediate_Replacement,$i_interp,$i_mover,$i_tot" >> $OUTPUT_CSV
    done
done
rm temp.txt
echo "Done! Results saved to $OUTPUT_CSV"