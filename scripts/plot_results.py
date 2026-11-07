import csv
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

project_dir = Path(__file__).resolve().parent.parent
csv_file = project_dir / "results" / "benchmark_results.csv"
output_file = project_dir / "results" / "graphs" / "mean_latency_comparison.png"

data = defaultdict(list)

with csv_file.open("r", newline="") as file:
    reader = csv.DictReader(file)

    for row in reader:
        key = (row["resolution"], row["mode"])
        data[key].append(float(row["mean_ms"]))

resolutions = ["640x480", "1280x720", "3840x2160"]
modes = ["CPU", "OpenCL"]

averages = {
    resolution: {
        mode: sum(data[(resolution, mode)]) / len(data[(resolution, mode)])
        for mode in modes
    }
    for resolution in resolutions
}

x = list(range(len(resolutions)))
width = 0.35

cpu_values = [averages[r]["CPU"] for r in resolutions]
opencl_values = [averages[r]["OpenCL"] for r in resolutions]

plt.figure(figsize=(10, 6))

plt.bar(
    [i - width / 2 for i in x],
    cpu_values,
    width,
    label="CPU"
)

plt.bar(
    [i + width / 2 for i in x],
    opencl_values,
    width,
    label="OpenCL"
)

plt.xlabel("Image Resolution")
plt.ylabel("Average Processing Latency (ms)")
plt.title("CPU vs OpenCL — Mean Latency Comparison")
plt.xticks(x, resolutions)
plt.legend()
plt.grid(axis="y", linestyle="--", alpha=0.5)

plt.tight_layout()
plt.savefig(output_file, dpi=300)
plt.close()

print(f"Graph saved to: {output_file}")

for resolution in resolutions:
    print(
        f"{resolution}: "
        f"CPU = {averages[resolution]['CPU']:.3f} ms, "
        f"OpenCL = {averages[resolution]['OpenCL']:.3f} ms"
    )

