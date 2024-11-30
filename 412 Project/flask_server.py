from flask import Flask, request
import time
import csv
import os

app = Flask(__name__)

last_request_time = None  # This will store the time of the last request

# CSV file to store the data
DATA_FILE = "data_log.csv"

# Ensure the CSV file has headers only if the file is created
def initialize_csv():
    if not os.path.exists(DATA_FILE):  # Only create the file and write headers if it doesn't exist
        with open(DATA_FILE, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(["temperature", "humidity", "pressure", "time_since_last"])
        print(f"Created new file: {DATA_FILE}")
    else:
        print(f"File {DATA_FILE} already exists. Appending data.")

# Calculate time since last request
def get_time_since_last_request():
    global last_request_time
    if last_request_time is None:
        last_request_time = time.time()  # First request, set the time
        return 0  # No previous request, so return 0
    else:
        current_time = time.time()
        time_diff = current_time - last_request_time
        last_request_time = current_time  # Update last request time
        return time_diff

# Flask route to handle incoming data
@app.route('/send-data', methods=['POST'])
def send_data():
    try:
        data = request.get_json()
        temperature = data['temperature']
        humidity = data['humidity']
        pressure = data['pressure']
        time_since_last = get_time_since_last_request()

        # Append data to the CSV file
        with open(DATA_FILE, mode='a', newline='') as file:  # Using 'a' mode to append
            writer = csv.writer(file)
            writer.writerow([temperature, humidity, pressure, time_since_last])

        return {"status": "success", "time_since_last": time_since_last}

    except Exception as e:
        return {"status": "error", "message": str(e)}, 400

# Initialize CSV file on server start
initialize_csv()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
