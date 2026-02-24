import pandas as pd
import matplotlib.pyplot as plt

# 1. Load the data from the CSV file
data = pd.read_csv("interpolation.csv")

# 2. Set up the plot
plt.figure(figsize=(10, 6))
bars = plt.bar(data['Configuration'], data['Time_Seconds'], color='skyblue', edgecolor='black')

# 3. Add titles and labels
plt.title('Serial Execution Time vs. Problem Configuration', fontsize=14)
plt.xlabel('Problem Index (Configuration)', fontsize=12)
plt.ylabel('Execution Time (Seconds)', fontsize=12)

# 4. Add the exact time numbers on top of each bar for clarity
for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + (yval * 0.01), round(yval, 4), ha='center', va='bottom', fontsize=10)

# 5. Add a grid behind the bars to make it easy to read
plt.grid(axis='y', linestyle='--', alpha=0.7)

# 6. Save the plot as an image file
plt.savefig('execution_time_plot.png', dpi=300)
print("Plot saved successfully as 'execution_time_plot.png'")
