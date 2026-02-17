import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv(
    "results/avg_algo_times.csv",
    skipinitialspace=True
)

# Rename columns 
df.columns = ["problem_size", "avg_e2e_time", "avg_algo_time"]

# ensure numeric types
for col in df.columns:
    df[col] = pd.to_numeric(df[col])

# Execution Time vs problem size
plt.figure()
plt.plot(
    np.log2(df["problem_size"]),
    np.log10(df["avg_algo_time"]),
    marker="o"
)
plt.xlabel("Problem Size (log2 scale)")
plt.ylabel("Execution Time (log10 scale)")
plt.title("Matrix Multiplication - Time vs Problem Size")
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.tight_layout()
plt.savefig("results/time_vs_problem_size.png")
plt.close()

print("Plots saved successfully:")
print(" - time_vs_problem_size.png")
