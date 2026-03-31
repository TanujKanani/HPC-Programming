import pandas as pd
import matplotlib.pyplot as plt
import os

CSV_FILE = 'final_particles.csv'

if not os.path.exists(CSV_FILE):
    print(f"Error: Could not find '{CSV_FILE}'.")
    exit()

print("Loading particle data...")
df = pd.read_csv(CSV_FILE)

# Set up a figure with 3 side-by-side plots
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 5))

# ---------------------------------------------------------
# Plot 1: Scatter Plot of Particle Positions
# ---------------------------------------------------------
# We sample a maximum of 20,000 points to prevent the plot from becoming 
# a solid block of color, allowing us to see the actual distribution.
sample_size = min(20000, len(df))
sample_df = df.sample(n=sample_size)

ax1.scatter(sample_df['x'], sample_df['y'], s=1, alpha=0.5, c='#1f77b4')
ax1.set_xlim(0, 1)
ax1.set_ylim(0, 1)
ax1.set_title(f"Particle Position Scatter Plot\n(Sampled {sample_size} points)", fontweight='bold')
ax1.set_xlabel("X Coordinate")
ax1.set_ylabel("Y Coordinate")
ax1.grid(True, linestyle='--', alpha=0.5)

# ---------------------------------------------------------
# Plot 2: 1D Histograms of X and Y coordinates
# ---------------------------------------------------------
# This proves that neither axis has a bias.
ax2.hist(df['x'], bins=50, alpha=0.6, color='red', label='X coordinates', histtype='stepfilled')
ax2.hist(df['y'], bins=50, alpha=0.6, color='green', label='Y coordinates', histtype='stepfilled')
ax2.set_title("1D Histograms of X and Y", fontweight='bold')
ax2.set_xlabel("Coordinate Value [0.0 to 1.0]")
ax2.set_ylabel("Number of Particles")
ax2.legend()
ax2.grid(True, linestyle='--', alpha=0.5)

# ---------------------------------------------------------
# Plot 3: Cell-wise Particle Counts (2D Heatmap)
# ---------------------------------------------------------
# This proves there are no artificial "hotspots" or "dead zones".
h = ax3.hist2d(df['x'], df['y'], bins=[50, 50], cmap='viridis')
fig.colorbar(h[3], ax=ax3, label='Particles per Cell')
ax3.set_title("Cell-wise Particle Counts (Heatmap)", fontweight='bold')
ax3.set_xlabel("X Coordinate")
ax3.set_ylabel("Y Coordinate")

# Save the figure
plt.tight_layout()
plt.savefig('Particle_Distribution_Verification.png', dpi=300, bbox_inches='tight')
print("Successfully created: Particle_Distribution_Verification.png")