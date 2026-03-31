import pandas as pd
import matplotlib.pyplot as plt
import os

LAB_CSV = 'experiment2_results_lab.csv'
HPC_CSV = 'experiment2_results_hpc_both.csv'

if not os.path.exists(LAB_CSV) or not os.path.exists(HPC_CSV):
    print(f"Error: Missing data files.")
    print(f"Please ensure both '{LAB_CSV}' and '{HPC_CSV}' are in this directory.")
    exit()

df_lab = pd.read_csv(LAB_CSV)
df_hpc = pd.read_csv(HPC_CSV)

# =============================================================================
# THE FIX: Standardize the approach names from older script runs
# This maps 'New_Insertion_Deletion' to 'Immediate_Replacement' seamlessly
# =============================================================================
df_lab['Approach'] = df_lab['Approach'].replace('New_Insertion_Deletion', 'Immediate_Replacement')
df_hpc['Approach'] = df_hpc['Approach'].replace('New_Insertion_Deletion', 'Immediate_Replacement')

grids = [(250, 100), (500, 200), (1000, 400)]
approaches = ['Assign04_Baseline', 'Immediate_Replacement', 'Deferred_Insertion']

print("Calculating speedups and generating combined plots...")

for nx, ny in grids:
    plt.figure(figsize=(12, 8))

    # Plot Ideal Speedup
    ideal_threads = [1, 2, 4, 8, 16]
    plt.plot(ideal_threads, ideal_threads, linestyle=':', color='gray', label='Ideal Linear Speedup')

    def plot_environment_data(df, env_name, line_style):
        for approach in approaches:
            subset = df[(df['Grid_Nx'] == nx) & (df['Grid_Ny'] == ny) & (df['Approach'] == approach)].copy()
            
            # If the approach doesn't exist (like Deferred on Lab PC), just skip it cleanly
            if subset.empty: 
                continue
                
            subset = subset.sort_values(by='Threads')
            
            t1_row = subset[subset['Threads'] == 1]
            if t1_row.empty: 
                continue
            t1_time = t1_row['Mover_Time_s'].values[0]
            
            subset['Speedup'] = t1_time / subset['Mover_Time_s']
            
            # Styling
            if approach == 'Assign04_Baseline':
                label = f'{env_name} - Baseline (Assign 04)'
                color = '#1f77b4' if env_name == 'Lab PC' else '#000080' 
                marker = 'o'
            elif approach == 'Immediate_Replacement':
                label = f'{env_name} - Immediate Replacement'
                color = '#ff7f0e' if env_name == 'Lab PC' else '#8b0000' 
                marker = 's'
            else: # Deferred
                label = f'{env_name} - Deferred Insertion'
                color = '#2ca02c' if env_name == 'Lab PC' else '#006400' 
                marker = '^'
                
            plt.plot(subset['Threads'], subset['Speedup'], 
                     marker=marker, linestyle=line_style, color=color, linewidth=2, label=label)

    # Plot data
    plot_environment_data(df_lab, 'Lab PC', '-')
    plot_environment_data(df_hpc, 'HPC Cluster', '--')

    # Formatting
    plt.xlabel('Number of Threads', fontsize=12, fontweight='bold')
    plt.ylabel('Speedup (T1 / Tn)', fontsize=12, fontweight='bold')
    plt.title(f'Experiment 02: OpenMP Scalability Comparison\nGrid Configuration: {nx} x {ny} (14 Million Particles)', fontsize=14)
    
    plt.xticks([1, 2, 4, 8, 16])
    plt.grid(True, which="both", ls="--", alpha=0.5)
    
    plt.legend(fontsize=10, loc='center left', bbox_to_anchor=(1, 0.5))

    filename = f'Exp2_Combined_Speedup_{nx}x{ny}.png'
    plt.tight_layout()
    plt.savefig(filename, dpi=300, bbox_inches='tight') 
    print(f"Successfully created: {filename}")

    plt.close()

print("All combined OpenMP scaling plots generated successfully!")