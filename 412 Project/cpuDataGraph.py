import pandas as pd
import matplotlib.pyplot as plt

# Path to your CSV file
csv_file = "cpu_data.csv"

# Read the CSV file
df = pd.read_csv(csv_file)

# Convert timestamp to datetime for better plotting
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Create the plot for freeHeap vs time
plt.figure(figsize=(12, 6))
plt.plot(df['timestamp'], df['freeHeap'], label="Free Heap (Bytes)", color='blue')

# Set plot title and labels
plt.title("Free Heap Over Time", fontsize=16)
plt.xlabel("Timestamp", fontsize=12)
plt.ylabel("Free Heap (Bytes)", fontsize=12)

# Rotate x-axis labels for better readability
plt.xticks(rotation=45)

# Add a grid and legend
plt.grid(True)
plt.legend()

# Show the plot
plt.tight_layout()
plt.show()
