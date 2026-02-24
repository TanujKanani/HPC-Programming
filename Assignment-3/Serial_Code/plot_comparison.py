import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df_pc = pd.read_csv("interpolation_PC.csv")
df_cluster = pd.read_csv("interpolation_cluster.csv")

plt.figure(figsize=(12, 7))

x = np.arange(len(df_pc['Configuration'])) 
width = 0.35

bars_pc = plt.bar(x - width/2, df_pc['Time_Seconds'], width, label='Lab PC', color='skyblue', edgecolor='black')
bars_cluster = plt.bar(x + width/2, df_cluster['Time_Seconds'], width, label='HPC Cluster', color='salmon', edgecolor='black')

plt.title('Serial Execution Time: Lab PC vs. HPC Cluster', fontsize=16)
plt.xlabel('Problem Index (Configuration)', fontsize=14)
plt.ylabel('Execution Time (Seconds)', fontsize=14)
plt.xticks(x, df_pc['Configuration'], fontsize=12)

def attach_labels(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + (height * 0.02),
                 f'{height:.2f}', ha='center', va='bottom', fontsize=10)

attach_labels(bars_pc)
attach_labels(bars_cluster)

plt.legend(fontsize=12)
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.tight_layout()
plt.savefig('execution_time_comparison.png', dpi=300)
print("Plot successfully saved as 'execution_time_comparison.png'")