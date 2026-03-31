import pandas as pd
import matplotlib.pyplot as plt
import os

LAB_CSV = 'experiment1_results_lab.csv'
HPC_CSV = 'experiment1_results_hpc.csv'

if not os.path.exists(LAB_CSV) or not os.path.exists(HPC_CSV):
    print(f"Error: Missing Experiment 1 data files.")
    exit()

df_lab = pd.read_csv(LAB_CSV)
df_hpc = pd.read_csv(HPC_CSV)

# We will analyze just the 'Immediate' approach to keep the trend line clean
df_lab = df_lab[df_lab['Approach'] == 'Immediate'].copy()
df_hpc = df_hpc[df_hpc['Approach'] == 'Immediate'].copy()

print("Calculating PPC and Per-Particle execution times...")

def calculate_metrics(df):
    # PPC = Total Particles / Total Cells
    df['PPC'] = df['Num_Particles'] / (df['Grid_Nx'] * df['Grid_Ny'])
    # Per-Particle Time = Total Execution Time / Total Particles
    df['Time_Per_Particle'] = df['Total_Time_s'] / df['Num_Particles']
    return df.sort_values(by='PPC')

df_lab = calculate_metrics(df_lab)
df_hpc = calculate_metrics(df_hpc)

plt.figure(figsize=(10, 6))

# Plot Lab PC (Solid Line)
plt.plot(df_lab['PPC'], df_lab['Time_Per_Particle'], 
         marker='o', linestyle='-', color='blue', linewidth=2, label='Lab PC')

# Plot HPC Cluster (Dashed Line)
plt.plot(df_hpc['PPC'], df_hpc['Time_Per_Particle'], 
         marker='s', linestyle='--', color='red', linewidth=2, label='HPC Cluster')

# Formatting
plt.xscale('log')
plt.yscale('log')

plt.xlabel('Particles Per Cell (PPC) [log scale]', fontsize=12, fontweight='bold')
plt.ylabel('Per-Particle Execution Time [s] (log scale)', fontsize=12, fontweight='bold')
plt.title('Performance Analysis: Per-Particle Execution Time vs. PPC', fontsize=14)

plt.grid(True, which="both", ls="--", alpha=0.5)
plt.legend(fontsize=10)

filename = 'Analysis_PPC_Trend.png'
plt.tight_layout()
plt.savefig(filename, dpi=300)
print(f"Successfully created: {filename}")