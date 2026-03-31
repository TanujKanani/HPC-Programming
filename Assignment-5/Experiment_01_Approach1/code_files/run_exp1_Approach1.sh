#!/bin/bash
OUTPUT_CSV="results_exp1_deferred.csv"
echo "Grid_Nx,Grid_Ny,Num_Particles,Approach,Interp_Time_s,Mover_Time_s,Total_Time_s" > $OUTPUT_CSV

GRIDS=( "250 100" "500 200" "1000 400" )
PARTICLES=( 100 10000 1000000 100000000 1000000000 )

echo "Compiling Exp 1 - Deferred..."
g++ -O3 -march=native -fopenmp exp1_deferred.cpp utils.cpp init.cpp -o run_sim

for grid in "${GRIDS[@]}"; do
    read NX NY <<< "$grid"
    for p in "${PARTICLES[@]}"; do
        ./run_sim $NX $NY $p > temp.txt
        line=$(grep "Deferred" temp.txt)
        interp=$(echo "$line" | awk -F'|' '{print $2}')
        mover=$(echo "$line" | awk -F'|' '{print $3}')
        total=$(echo "$line" | awk -F'|' '{print $4}')
        echo "$NX,$NY,$p,Deferred,$interp,$mover,$total" >> $OUTPUT_CSV
    done
done
rm temp.txt
echo "Done! Results saved to $OUTPUT_CSV"s