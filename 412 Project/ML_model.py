import pandas as pd
import numpy as np
import json
import matplotlib.pyplot as plt
from sklearn.metrics import silhouette_score
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import KMeans

# Load the data from the CSV file
df = pd.read_csv('data_log.csv')

# Select the features (temperature, humidity, pressure, time_since_last)
features = df[['temperature', 'humidity', 'pressure', 'time_since_last']]

# Scale the data
scaler = StandardScaler()
scaled_features = scaler.fit_transform(features)

# Print the scaled features to check
print(scaled_features[:5])  # Print the first 5 rows

# Print the mean and standard deviation of the scaled features
print("Means: ", scaled_features.mean(axis=0))  # Should be close to 0
print("Standard deviations: ", scaled_features.std(axis=0))  # Should be close to 1

# Perform K-means clustering with k=3
kmeans = KMeans(n_clusters=3, init='k-means++', max_iter=300, n_init=10, random_state=42)
kmeans.fit(scaled_features)

# Add the cluster labels to the original DataFrame
df['cluster'] = kmeans.labels_

# Print the first few rows of the DataFrame with the cluster labels
print(df.head())

# Optionally, save the results to a new CSV file
df.to_csv('data_with_clusters.csv', index=False)

# NEXT STEP ---------------------------------------------------
# Inertia: Sum of squared distances of samples to their closest cluster center
print("Inertia (sum of squared distances):", kmeans.inertia_)

# Calculate silhouette score
sil_score = silhouette_score(features, kmeans.labels_)
print("Silhouette Score:", sil_score)

# DETECTING ANOMALIES -----------------------------------------
# Calculate distances from each point to its assigned cluster center
distances = kmeans.transform(features)  # This gives the distance to each cluster center

# Get the minimum distance (i.e., the closest cluster center for each data point)
min_distances = np.min(distances, axis=1)

# Define a threshold for anomaly detection, you can adjust this value
threshold = np.percentile(min_distances, 90)  # e.g., top 10% as anomalies

# Mark anomalies
df['anomaly'] = min_distances > threshold

# Print the data points marked as anomalies
print(df[df['anomaly'] == True])

# CENTROIDS ------------------------------------------------------
# Get the centroids of the clusters
centroids = kmeans.cluster_centers_

# Export the centroids to a CSV file
centroids_df = pd.DataFrame(centroids, columns=['temperature', 'humidity', 'pressure', 'time_since_last'])
centroids_df.to_csv('centroids.csv', index=False)

centroids_json = centroids.tolist()
with open('centroids.json', 'w') as f:
    json.dump(centroids_json, f)

# FIGURE CODE -------------------------------------------------
# Plotting the clusters with anomalies highlighted
plt.figure(figsize=(10, 6))

# Plot all points first
plt.scatter(df['temperature'], df['humidity'], c=df['cluster'], cmap='viridis', label='Normal Data')

# Highlight anomalies
plt.scatter(df[df['anomaly'] == True]['temperature'], 
            df[df['anomaly'] == True]['humidity'], 
            color='red', label='Anomalies', marker='x')

# Adding labels and title
plt.xlabel('Temperature')
plt.ylabel('Humidity')
plt.title('Clustered Data with Anomalies Highlighted')
plt.colorbar(label='Cluster')

# Show legend
plt.legend()

# Show the plot
plt.show()


