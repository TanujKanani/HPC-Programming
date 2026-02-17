import os
import csv
import subprocess
from collections import defaultdict

# User-configurable parameters
EXECUTABLE = "./output"               # compiled binary file
NUM_RUNS = 5                          # number of repetitions
OUTPUT_CSV = "results/avg_algo_times.csv"

# Ensure results directory exists
os.makedirs(os.path.dirname(OUTPUT_CSV), exist_ok=True)

# Storage for results (keyed by problem size)
results = defaultdict(lambda: {
    "e2e_times": [],
    "algo_times": []
})

print(f"Running benchmark {NUM_RUNS} times...\n")

# Run executable multiple times
for run_id in range(NUM_RUNS):
    print(f"\n=== Run {run_id + 1}/{NUM_RUNS} ===")

    proc = subprocess.Popen(
        [EXECUTABLE],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1  # line-buffered
    )

    header_seen = False

    while True:
        line = proc.stdout.readline()
        if not line and proc.poll() is not None:
            break

        if not line:
            continue

        line = line.strip()
        print(line)  

        # Skip header line
        if "ProblemSize" in line:
            header_seen = True
            continue

        if not header_seen:
            continue

        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 3:
            continue

        # Parse values
        problem_size = int(parts[0])
        e2e_time = float(parts[1])
        algo_time = float(parts[2])

        results[problem_size]["e2e_times"].append(e2e_time)
        results[problem_size]["algo_times"].append(algo_time)

    # print stderr if any
    err = proc.stderr.read()
    if err:
        print("STDERR:", err)

# Write averaged results to CSV
with open(OUTPUT_CSV, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "ProblemSize",
        "AvgE2ETime",
        "AvgAlgoTime"
    ])

    for problem_size in sorted(results.keys()):
        entry = results[problem_size]

        avg_e2e = sum(entry["e2e_times"]) / len(entry["e2e_times"])
        avg_algo = sum(entry["algo_times"]) / len(entry["algo_times"])

        writer.writerow([
            problem_size,
            f"{avg_e2e:.9f}",
            f"{avg_algo:.9f}"
        ])

print(f"\nAveraged results written to: {OUTPUT_CSV}")
