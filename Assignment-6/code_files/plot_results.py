#!/usr/bin/env python3
"""
HPC Assignment 06 - Plotting Script
Generates Speedup vs Cores and Execution Time vs Cores plots
for both Lab PC and Cluster results.
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import os

matplotlib.rcParams['font.family'] = 'DejaVu Sans'
matplotlib.rcParams['font.size'] = 11

# ─── Output directory ───
OUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ============================================================================
#  1.  CLUSTER DATA  (already has serial row at Threads=1)
# ============================================================================
cluster = pd.read_csv(os.path.join(OUT_DIR, "results_cluster.csv"))
cluster.columns = cluster.columns.str.strip()

# Serial time per config (Threads == 1)
serial_cluster = cluster[cluster["Threads"] == 1].set_index("Config")["TotalTime_sec"]

cluster["Speedup"] = cluster.apply(
    lambda r: serial_cluster[r["Config"]] / r["TotalTime_sec"], axis=1
)
cluster["Efficiency"] = cluster["Speedup"] / cluster["Threads"]

# ============================================================================
#  2.  LAB PC DATA  (separate Serial / Parallel rows, different thread counts)
# ============================================================================
pc = pd.read_csv(os.path.join(OUT_DIR, "results_combined_PC.csv"))
pc.columns = pc.columns.str.strip()

# Extract config label from filename  "input_A.bin" -> "A"
pc["Config"] = pc["Config"].str.extract(r"input_(\w)\.bin")

# Serial baselines
serial_pc = pc[pc["Mode"] == "Serial"].set_index("Config")["Time"]

# Keep only parallel rows for plots
pc_par = pc[pc["Mode"] == "Parallel"].copy()
pc_par["Speedup"] = pc_par.apply(
    lambda r: serial_pc[r["Config"]] / r["Time"], axis=1
)
pc_par["Efficiency"] = pc_par["Speedup"] / pc_par["Threads"]

# Also build a combined df that includes the serial row (Thread=1, Speedup=1)
pc_all = []
for cfg in serial_pc.index:
    pc_all.append({"Config": cfg, "Threads": 1,
                   "Time": serial_pc[cfg], "Speedup": 1.0, "Efficiency": 1.0})
pc_all = pd.concat([pd.DataFrame(pc_all), pc_par], ignore_index=True)
pc_all.sort_values(["Config", "Threads"], inplace=True)

configs = sorted(cluster["Config"].unique())
colors  = {c: col for c, col in zip(configs, plt.cm.tab10.colors)}

# ============================================================================
#  HELPER:  Plot a metric for one data-source
# ============================================================================
def plot_metric(df, x_col, y_col, title, ylabel, fname, ideal=False):
    fig, ax = plt.subplots(figsize=(9, 5.5))
    threads_all = sorted(df[x_col].unique())

    for cfg in configs:
        sub = df[df["Config"] == cfg].sort_values(x_col)
        ax.plot(sub[x_col], sub[y_col], "o-", label=f"Config {cfg}",
                color=colors[cfg], linewidth=2, markersize=6)

    if ideal and y_col == "Speedup":
        ax.plot(threads_all, threads_all, "k--", label="Ideal (linear)", linewidth=1.5)
    if ideal and y_col == "Efficiency":
        ax.axhline(1.0, color="k", linestyle="--", label="Ideal (100%)", linewidth=1.5)

    ax.set_xlabel("Number of Threads (Cores)", fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=14, fontweight="bold")
    ax.set_xticks(threads_all)
    ax.legend(fontsize=9, loc="best")
    ax.grid(True, alpha=0.35)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT_DIR, fname), dpi=180)
    print(f"  Saved {fname}")
    plt.close(fig)

# ============================================================================
#  GENERATE ALL PLOTS
# ============================================================================
print("=== Generating Cluster plots ===")
plot_metric(cluster, "Threads", "TotalTime_sec",
            "Cluster – Execution Time vs Cores",
            "Total Execution Time (s)", "cluster_time_vs_cores.png")

plot_metric(cluster, "Threads", "Speedup",
            "Cluster – Speedup vs Cores",
            "Speedup (T₁ / Tₚ)", "cluster_speedup_vs_cores.png", ideal=True)

plot_metric(cluster, "Threads", "Efficiency",
            "Cluster – Parallel Efficiency vs Cores",
            "Efficiency (Speedup / P)", "cluster_efficiency_vs_cores.png", ideal=True)

print("\n=== Generating Lab PC plots ===")
plot_metric(pc_all, "Threads", "Time",
            "Lab PC – Execution Time vs Cores",
            "Total Execution Time (s)", "pc_time_vs_cores.png")

plot_metric(pc_all, "Threads", "Speedup",
            "Lab PC – Speedup vs Cores",
            "Speedup (T₁ / Tₚ)", "pc_speedup_vs_cores.png", ideal=True)

plot_metric(pc_all, "Threads", "Efficiency",
            "Lab PC – Parallel Efficiency vs Cores",
            "Efficiency (Speedup / P)", "pc_efficiency_vs_cores.png", ideal=True)

# ============================================================================
#  PRINT SUMMARY TABLES  (for the report)
# ============================================================================
print("\n" + "="*80)
print("CLUSTER – Speedup & Efficiency Summary")
print("="*80)
for cfg in configs:
    sub = cluster[cluster["Config"] == cfg].sort_values("Threads")
    print(f"\nConfig {cfg}  (NX={sub.iloc[0]['NX']}, NY={sub.iloc[0]['NY']}, "
          f"Points={sub.iloc[0]['Points']:,}, Maxiter={sub.iloc[0]['Maxiter']})")
    print(f"  {'Threads':>7s}  {'Time(s)':>10s}  {'Speedup':>8s}  {'Efficiency':>10s}")
    for _, r in sub.iterrows():
        print(f"  {int(r['Threads']):7d}  {r['TotalTime_sec']:10.6f}  "
              f"{r['Speedup']:8.3f}  {r['Efficiency']:10.3f}")

print("\n" + "="*80)
print("LAB PC – Speedup & Efficiency Summary")
print("="*80)
for cfg in configs:
    sub = pc_all[pc_all["Config"] == cfg].sort_values("Threads")
    print(f"\nConfig {cfg}")
    print(f"  {'Threads':>7s}  {'Time(s)':>10s}  {'Speedup':>8s}  {'Efficiency':>10s}")
    for _, r in sub.iterrows():
        print(f"  {int(r['Threads']):7d}  {r['Time']:10.6f}  "
              f"{r['Speedup']:8.3f}  {r['Efficiency']:10.3f}")

print("\nDone! All plots saved to:", OUT_DIR)
