#!/bin/bash

# Ensure both programs are compiled
g++ input_file_maker.cpp -o input_maker
g++ main.cpp init.cpp utils_fast2.cpp -o hpc_interp -O3

# Create the CSV file and add the header row
CSV_FILE="interpolation.csv"
echo "Configuration,Points_Millions,Time_Seconds" > $CSV_FILE

# A simple function to run the experiment and save the time
run_exp() {
    local config=$1
    local nx=$2
    local ny=$3
    local pts=$4
    local pts_label=$5

    echo "======================================"
    echo "Running Configuration $config (Nx=$nx, Ny=$ny, Points=$pts_label)"

    # Generate input file
    echo "$nx $ny $pts 10" | ./input_maker > /dev/null

    # Run the program and catch the output
    local output=$(./hpc_interp input.bin)
    echo "$output"

    # Extract just the number (the 6th word in the output sentence)
    local time_val=$(echo "$output" | grep "Total interpolation time" | awk '{print $6}')

    # Save to CSV
    echo "$config,$pts_label,$time_val" >> $CSV_FILE
}

# Run all 5 configurations
run_exp "A" 250 100 900000 "0.9"
run_exp "B" 250 100 5000000 "5.0"
run_exp "C" 500 200 3600000 "3.6"
run_exp "D" 500 200 20000000 "20.0"
run_exp "E" 1000 400 14000000 "14.0"

echo "======================================"
echo "All experiments completed! Data saved to $CSV_FILE"
