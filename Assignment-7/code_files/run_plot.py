import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import os

try:
    df = pd.read_csv('results.csv', sep=';')
except FileNotFoundError:
    print("Error: results.csv not found. Please run the bash script first.")
    exit()

configs = df['scenario'].unique()

output_dir = "Assignment_Plots"
os.makedirs(output_dir, exist_ok=True)

print(f"Generating plots for {len(configs)} configurations...")

mpl.rcParams.update({
    'font.family': 'serif',
    'font.size': 11,
    'axes.titlesize': 13,
    'axes.labelsize': 11,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'axes.spines.top': False,
    'axes.spines.right': False,
})

palette = {
    'A': '#2b6cb0',
    'B': '#38a169',
    'C': '#c53030',
    'D': '#b7791f',
    'E': '#6b46c1',
}

fig_all_exec, ax_all_exec = plt.subplots(figsize=(9, 5.5))
fig_all_sp, ax_all_sp = plt.subplots(figsize=(9, 5.5))
fig_all_ph, ax_all_ph = plt.subplots(figsize=(11, 6))

for config in configs:
    subset = df[df['scenario'] == config]

    t_serial = subset[subset['threads'] == 1]['walltime_alg'].values[0]

    cores = subset['threads'].values
    t_total = subset['walltime_alg'].values
    t_int = subset['walltime_interp'].values
    t_mov = subset['walltime_mover'].values

    sp = t_serial / t_total

    clr = palette.get(config, '#555555')

    ax_all_exec.plot(cores, t_total, marker='D', color=clr, lw=1.8, ms=6, label=f'Config {config}')
    ax_all_sp.plot(cores, sp, marker='v', color=clr, lw=1.8, ms=6, label=f'Config {config}')

    ax_all_ph.plot(cores, t_int, marker='D', ls='-', color=clr, lw=1.6, ms=5, label=f'{config} – Interp')
    ax_all_ph.plot(cores, t_mov, marker='x', ls=':', color=clr, lw=1.6, ms=6, label=f'{config} – Mover')

    fig1, ax1 = plt.subplots(figsize=(8, 5))
    ax1.plot(cores, t_total, marker='D', color='#2b6cb0', lw=1.8, ms=7)
    ax1.fill_between(cores, t_total, alpha=0.08, color='#2b6cb0')
    ax1.set_title(f'Config {config} – Execution Time vs Cores', fontweight='bold')
    ax1.set_xlabel('Cores')
    ax1.set_ylabel('Time (s)')
    ax1.set_xticks(cores)
    ax1.grid(axis='y', ls=':', alpha=0.5)
    fig1.tight_layout()
    fig1.savefig(f'{output_dir}/Execution_Time_Config_{config}.png')
    plt.close(fig1)

    fig2, ax2 = plt.subplots(figsize=(8, 5))
    ax2.plot(cores, sp, marker='v', color='#38a169', lw=1.8, ms=7, label='Measured')
    ax2.plot(cores, cores, ls='--', color='grey', lw=1.2, label='Ideal (linear)')
    ax2.set_title(f'Config {config} – Speedup vs Cores', fontweight='bold')
    ax2.set_xlabel('Cores')
    ax2.set_ylabel('Speedup (T₁ / Tₙ)')
    ax2.set_xticks(cores)
    ax2.grid(axis='y', ls=':', alpha=0.5)
    ax2.legend(frameon=False)
    fig2.tight_layout()
    fig2.savefig(f'{output_dir}/Speedup_Config_{config}.png')
    plt.close(fig2)

    fig3, ax3 = plt.subplots(figsize=(8, 5))
    ax3.plot(cores, t_int, marker='D', color='#c53030', lw=1.8, ms=7, label='Interpolation')
    ax3.plot(cores, t_mov, marker='x', color='#6b46c1', lw=1.8, ms=7, label='Mover')
    ax3.set_title(f'Config {config} – Phase Breakdown', fontweight='bold')
    ax3.set_xlabel('Cores')
    ax3.set_ylabel('Time (s)')
    ax3.set_xticks(cores)
    ax3.grid(axis='y', ls=':', alpha=0.5)

    frac_int = t_int.sum() / (t_int.sum() + t_mov.sum()) * 100
    ax3.annotate(f'Interpolation ≈ {frac_int:.1f}% of core time',
                 xy=(0.97, 0.92), xycoords='axes fraction', ha='right',
                 fontsize=10, fontstyle='italic',
                 bbox=dict(boxstyle='round,pad=0.3', fc='#fefcbf', ec='#b7791f', lw=0.8))

    ax3.legend(frameon=False)
    fig3.tight_layout()
    fig3.savefig(f'{output_dir}/Phase_Analysis_Config_{config}.png')
    plt.close(fig3)

ax_all_exec.set_title('All Configs – Execution Time', fontweight='bold')
ax_all_exec.set_xlabel('Cores')
ax_all_exec.set_ylabel('Time (s)')
ax_all_exec.set_xticks(cores)
ax_all_exec.grid(axis='y', ls=':', alpha=0.5)
ax_all_exec.legend(frameon=False)
fig_all_exec.tight_layout()
fig_all_exec.savefig(f'{output_dir}/Combined_Execution_Time.png')
plt.close(fig_all_exec)

ax_all_sp.plot(cores, cores, ls='--', color='grey', lw=1.2, label='Ideal')
ax_all_sp.set_title('All Configs – Speedup', fontweight='bold')
ax_all_sp.set_xlabel('Cores')
ax_all_sp.set_ylabel('Speedup (T₁ / Tₙ)')
ax_all_sp.set_xticks(cores)
ax_all_sp.grid(axis='y', ls=':', alpha=0.5)
ax_all_sp.legend(frameon=False)
fig_all_sp.tight_layout()
fig_all_sp.savefig(f'{output_dir}/Combined_Speedup.png')
plt.close(fig_all_sp)

ax_all_ph.set_title('All Configs – Interpolation vs Mover', fontweight='bold')
ax_all_ph.set_xlabel('Cores')
ax_all_ph.set_ylabel('Time (s)')
ax_all_ph.set_xticks(cores)
ax_all_ph.grid(axis='y', ls=':', alpha=0.5)
ax_all_ph.legend(bbox_to_anchor=(1.02, 1), loc='upper left', frameon=False, fontsize=9)
fig_all_ph.tight_layout()
fig_all_ph.savefig(f'{output_dir}/Combined_Phase_Analysis.png')
plt.close(fig_all_ph)

print(f"Done – 18 plots saved to {output_dir}/")
