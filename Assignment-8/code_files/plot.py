"""
plot_results.py
---------------
Generates all plots required by HPC Assignment 8:
  Q3  -> execution_time_vs_cores.png
       -> speedup_vs_cores.png
  Q4  -> parallel_efficiency_vs_cores.png
  Q5  -> max_speedup_bar.png          (vs serial baseline)
  Q11 -> phase_breakdown_vs_cores.png (interp vs mover per config)
         interp_vs_mover_bar.png      (total time split, all configs)

Usage:
  python3 plot_results.py                  # uses results_all.csv
  python3 plot_results.py my_results.csv   # custom file

CSV must have columns:
  config, cores, ranks, threads_per_rank, wall_time_s, interp_time_s, mover_time_s

Serial baseline:
  If a row with cores=1 exists it is used as the serial reference.
  Otherwise cores=2 is used as the baseline (speedup relative to 2 cores).
"""

import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# ── config ───────────────────────────────────────────────────────────────────
CSV_FILE  = sys.argv[1] if len(sys.argv) > 1 else "results_all.csv"
OUT_DIR   = "plots"
DPI       = 150

CONFIGS   = ["a_250x100_900k", "b_250x100_5m",
             "c_500x200_3p6m", "d_500x200_20m", "e_1000x400_14m"]
LABELS    = ["A: 250×100, 0.9M", "B: 250×100, 5M",
             "C: 500×200, 3.6M", "D: 500×200, 20M", "E: 1000×400, 14M"]
COLORS    = ["#378ADD", "#1D9E75", "#D85A30", "#BA7517", "#7F77DD"]
MARKERS   = ["o", "s", "^", "D", "P"]

os.makedirs(OUT_DIR, exist_ok=True)

# ── load data ─────────────────────────────────────────────────────────────────
df = pd.read_csv(CSV_FILE)
df.columns = df.columns.str.strip()
df["config"] = df["config"].str.strip()

# Determine baseline: prefer cores=1, else cores=2
has_serial = 1 in df["cores"].values
baseline_cores = 1 if has_serial else 2
baseline_label = "serial (1 core)" if has_serial else "2-core run"

print(f"Loaded {len(df)} rows from {CSV_FILE}")
print(f"Configs found : {df['config'].unique().tolist()}")
print(f"Cores found   : {sorted(df['cores'].unique().tolist())}")
print(f"Baseline      : {baseline_label}")
print()

def get_config_df(cfg):
    return df[df["config"] == cfg].sort_values("cores")

def baseline_time(cfg, col="wall_time_s"):
    sub = get_config_df(cfg)
    row = sub[sub["cores"] == baseline_cores]
    if row.empty:
        row = sub.iloc[[0]]
    return row[col].values[0]

# ── helpers ───────────────────────────────────────────────────────────────────
def style_ax(ax, xlabel, ylabel, title):
    ax.set_xlabel(xlabel, fontsize=11)
    ax.set_ylabel(ylabel, fontsize=11)
    ax.set_title(title, fontsize=12, fontweight="bold")
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_formatter(ticker.ScalarFormatter())
    ax.tick_params(labelsize=10)

def save(fig, name):
    path = os.path.join(OUT_DIR, name)
    fig.savefig(path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")

# =============================================================================
# Plot 1 — Execution time vs cores  (Q3)
# =============================================================================
fig, ax = plt.subplots(figsize=(8, 5))

for cfg, label, color, marker in zip(CONFIGS, LABELS, COLORS, MARKERS):
    sub = get_config_df(cfg)
    if sub.empty:
        continue
    ax.plot(sub["cores"], sub["wall_time_s"],
            color=color, marker=marker, linewidth=2, markersize=7, label=label)

style_ax(ax, "Number of cores", "Wall time (s)", "Execution time vs cores")
ax.legend(fontsize=9, loc="upper right")
save(fig, "execution_time_vs_cores.png")

# =============================================================================
# Plot 2 — Speedup vs cores  (Q3)
# =============================================================================
all_cores = sorted(df["cores"].unique())
ideal_x   = [baseline_cores] + [c for c in all_cores if c > baseline_cores]

fig, ax = plt.subplots(figsize=(8, 5))

for cfg, label, color, marker in zip(CONFIGS, LABELS, COLORS, MARKERS):
    sub = get_config_df(cfg)
    if sub.empty:
        continue
    t0 = baseline_time(cfg)
    sub = sub[sub["cores"] >= baseline_cores].copy()
    sub["speedup"] = t0 / sub["wall_time_s"]
    ax.plot(sub["cores"], sub["speedup"],
            color=color, marker=marker, linewidth=2, markersize=7, label=label)

# Ideal speedup line
ideal_y = [c / baseline_cores for c in ideal_x]
ax.plot(ideal_x, ideal_y,
        color="gray", linestyle="--", linewidth=1.5, label="Ideal speedup")

style_ax(ax, "Number of cores", f"Speedup (relative to {baseline_label})",
         "Speedup vs cores")
ax.legend(fontsize=9, loc="upper left")
save(fig, "speedup_vs_cores.png")

# =============================================================================
# Plot 3 — Parallel efficiency vs cores  (Q4)
# =============================================================================
fig, ax = plt.subplots(figsize=(8, 5))

for cfg, label, color, marker in zip(CONFIGS, LABELS, COLORS, MARKERS):
    sub = get_config_df(cfg)
    if sub.empty:
        continue
    t0  = baseline_time(cfg)
    sub = sub[sub["cores"] >= baseline_cores].copy()
    sub["efficiency"] = (t0 / sub["wall_time_s"]) / (sub["cores"] / baseline_cores) * 100
    ax.plot(sub["cores"], sub["efficiency"],
            color=color, marker=marker, linewidth=2, markersize=7, label=label)

ax.axhline(100, color="gray", linestyle="--", linewidth=1.5, label="Ideal (100%)")
style_ax(ax, "Number of cores", "Parallel efficiency (%)",
         "Parallel efficiency vs cores")
ax.legend(fontsize=9)
ax.set_ylim(0, 120)
save(fig, "parallel_efficiency_vs_cores.png")

# =============================================================================
# Plot 4 — Max speedup bar chart  (Q5)
# =============================================================================
fig, ax = plt.subplots(figsize=(8, 5))

max_speedups = []
best_cores   = []
for cfg in CONFIGS:
    sub = get_config_df(cfg)
    if sub.empty:
        max_speedups.append(0); best_cores.append(0); continue
    t0   = baseline_time(cfg)
    sub2 = sub[sub["cores"] >= baseline_cores].copy()
    sub2["speedup"] = t0 / sub2["wall_time_s"]
    idx  = sub2["speedup"].idxmax()
    max_speedups.append(sub2.loc[idx, "speedup"])
    best_cores.append(int(sub2.loc[idx, "cores"]))

x = np.arange(len(CONFIGS))
bars = ax.bar(x, max_speedups, color=COLORS, edgecolor="white", width=0.55)
ax.set_xticks(x)
ax.set_xticklabels(LABELS, fontsize=9, rotation=10)
ax.set_ylabel("Maximum speedup", fontsize=11)
ax.set_title(f"Maximum speedup achieved (baseline: {baseline_label})",
             fontsize=12, fontweight="bold")
ax.grid(axis="y", linestyle="--", alpha=0.4)

for bar, sp, bc in zip(bars, max_speedups, best_cores):
    ax.text(bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 0.05,
            f"{sp:.2f}×\n@{bc}c",
            ha="center", va="bottom", fontsize=9, fontweight="bold")

save(fig, "max_speedup_bar.png")

# =============================================================================
# Plot 5 — Interpolation time vs cores  (Q11)
# =============================================================================
fig, ax = plt.subplots(figsize=(8, 5))

for cfg, label, color, marker in zip(CONFIGS, LABELS, COLORS, MARKERS):
    sub = get_config_df(cfg)
    if sub.empty or "interp_time_s" not in sub.columns:
        continue
    ax.plot(sub["cores"], sub["interp_time_s"],
            color=color, marker=marker, linewidth=2, markersize=7, label=label)

style_ax(ax, "Number of cores", "Interpolation time (s)",
         "Interpolation phase time vs cores  (Q11)")
ax.legend(fontsize=9)
save(fig, "interpolation_time_vs_cores.png")

# =============================================================================
# Plot 6 — Mover time vs cores  (Q11)
# =============================================================================
fig, ax = plt.subplots(figsize=(8, 5))

for cfg, label, color, marker in zip(CONFIGS, LABELS, COLORS, MARKERS):
    sub = get_config_df(cfg)
    if sub.empty or "mover_time_s" not in sub.columns:
        continue
    ax.plot(sub["cores"], sub["mover_time_s"],
            color=color, marker=marker, linewidth=2, markersize=7, label=label)

style_ax(ax, "Number of cores", "Mover time (s)",
         "Mover phase time vs cores  (Q11)")
ax.legend(fontsize=9)
save(fig, "mover_time_vs_cores.png")

# =============================================================================
# Plot 7 — Interp vs Mover stacked bar at each core count  (Q11)
#           One subplot per config
# =============================================================================
fig, axes = plt.subplots(2, 3, figsize=(14, 8))
axes = axes.flatten()

for idx, (cfg, label) in enumerate(zip(CONFIGS, LABELS)):
    ax  = axes[idx]
    sub = get_config_df(cfg)
    if sub.empty:
        ax.set_visible(False); continue

    cores_list = sub["cores"].values
    interp     = sub["interp_time_s"].values
    mover      = sub["mover_time_s"].values
    other      = np.maximum(sub["wall_time_s"].values - interp - mover, 0)

    x = np.arange(len(cores_list))
    w = 0.55
    ax.bar(x, interp, width=w, label="Interpolation", color="#378ADD")
    ax.bar(x, mover,  width=w, bottom=interp,         label="Mover",         color="#1D9E75")
    ax.bar(x, other,  width=w, bottom=interp + mover, label="Other/overhead",color="#B4B2A9")

    ax.set_xticks(x)
    ax.set_xticklabels([str(c) for c in cores_list], fontsize=8)
    ax.set_title(label, fontsize=10, fontweight="bold")
    ax.set_xlabel("Cores", fontsize=9)
    ax.set_ylabel("Time (s)", fontsize=9)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    if idx == 0:
        ax.legend(fontsize=8, loc="upper right")

axes[-1].set_visible(False)
fig.suptitle("Phase breakdown: interpolation vs mover vs overhead  (Q11)",
             fontsize=13, fontweight="bold", y=1.01)
fig.tight_layout()
save(fig, "phase_breakdown_stacked.png")

# =============================================================================
# Summary
# =============================================================================
print()
print("All plots saved to:", OUT_DIR)
print()
print("Files generated:")
for f in sorted(os.listdir(OUT_DIR)):
    print(f"  {f}")
