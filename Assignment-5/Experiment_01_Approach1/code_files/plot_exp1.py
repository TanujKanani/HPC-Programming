import pandas as pd
import matplotlib.pyplot as plt
import os

# File names for your data
LAB_CSV = 'experiment1_results_lab.csv'
HPC_CSV = 'experiment1_results_hpc.csv'

# Ensure the files exist before proceeding
if not os.path.exists(LAB_CSV) or not os.path.exists(HPC_CSV):
    print(f"Error: Missing data files.")
    print(f"Please ensure both '{LAB_CSV}' and '{HPC_CSV}' are in this directory.")
    exit()

# Load the data
df_lab = pd.read_csv(LAB_CSV)
df_hpc = pd.read_csv(HPC_CSV)

# The 3 grid configurations required by Assignment 05
grids = [(250, 100), (500, 200), (1000, 400)]

print("Generating plots...")

for nx, ny in grids:
    plt.figure(figsize=(10, 6))

    # Filter data for the current grid iteration
    lab_grid = df_lab[(df_lab['Grid_Nx'] == nx) & (df_lab['Grid_Ny'] == ny)]
    hpc_grid = df_hpc[(df_hpc['Grid_Nx'] == nx) & (df_hpc['Grid_Ny'] == ny)]

    # Extract Immediate and Deferred approaches for Lab PC
    lab_imm = lab_grid[lab_grid['Approach'] == 'Immediate']
    lab_def = lab_grid[lab_grid['Approach'] == 'Deferred']

    # Extract Immediate and Deferred approaches for HPC
    hpc_imm = hpc_grid[hpc_grid['Approach'] == 'Immediate']
    hpc_def = hpc_grid[hpc_grid['Approach'] == 'Deferred']

    # Plot Lab PC data (Solid lines, circle markers)
    plt.plot(lab_imm['Num_Particles'], lab_imm['Total_Time_s'], 
             marker='o', linestyle='-', color='blue', label='Lab PC - Immediate')
    plt.plot(lab_def['Num_Particles'], lab_def['Total_Time_s'], 
             marker='o', linestyle='-', color='cyan', label='Lab PC - Deferred')

    # Plot HPC data (Dashed lines, square markers)
    plt.plot(hpc_imm['Num_Particles'], hpc_imm['Total_Time_s'], 
             marker='s', linestyle='--', color='red', label='HPC Cluster - Immediate')
    plt.plot(hpc_def['Num_Particles'], hpc_def['Total_Time_s'], 
             marker='s', linestyle='--', color='orange', label='HPC Cluster - Deferred')

    # Apply logarithmic scales as requested
    plt.xscale('log')
    plt.yscale('log')

    # Labels and Formatting
    plt.xlabel('Number of Particles (log scale)', fontsize=12, fontweight='bold')
    plt.ylabel('Total Execution Time [s] (log scale)', fontsize=12, fontweight='bold')
    plt.title(f'Experiment 01: Execution Time vs. Particle Count\nGrid Configuration: {nx} x {ny}', fontsize=14)
    
    # Grid lines to make log scale easier to read
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(fontsize=10, loc='upper left')

    # Save the plot securely to the directory
    filename = f'Exp1_Plot_Grid_{nx}x{ny}.png'
    plt.tight_layout()
    plt.savefig(filename, dpi=300) # High DPI for clean report PDFs
    print(f"Successfully created: {filename}")

    plt.close()

print("All plots generated successfully. Ready for your report!")