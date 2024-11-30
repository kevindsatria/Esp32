import pandas as pd
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt
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

# Elbow Method: Finding the optimal number of clusters
wcss = []

for k in range(1, 11):  # Test for k from 1 to 10
    kmeans = KMeans(n_clusters=k, init='k-means++', max_iter=300, n_init=10, random_state=42)
    kmeans.fit(scaled_features)
    wcss.append(kmeans.inertia_)  # inertia_ is the WCSS for the model

# Plot the WCSS values for each k
plt.plot(range(1, 11), wcss)
plt.title('Elbow Method for Optimal k')
plt.xlabel('Number of Clusters (k)')
plt.ylabel('WCSS')
plt.show()
