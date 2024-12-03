from flask import Flask, request, jsonify
import csv

app = Flask(__name__)

# Path to CSV file
CSV_FILE = "cpu_data.csv"

# Always recreate the CSV file with headers when the server starts
with open(CSV_FILE, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["freeHeap", "timestamp"])

# Route to receive the data from ESP32
@app.route('/receive_data', methods=['POST'])
def receive_data():
    try:
        # Retrieve the JSON data sent by the ESP32
        data = request.get_json()

        # Extract the freeHeap value from the received JSON
        freeHeap = data.get("freeHeap")

        # Ensure freeHeap is an integer
        freeHeap = int(freeHeap) if freeHeap is not None else 0
        
        # Get the current timestamp
        from datetime import datetime
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Write the data to the CSV file
        with open(CSV_FILE, mode='a', newline='') as file:
            writer = csv.writer(file)
            writer.writerow([freeHeap, timestamp])

        # Return a success response
        return jsonify({"status": "success", "message": "Data received and stored."}), 200
    except Exception as e:
        # Return an error response
        return jsonify({"status": "error", "message": str(e)}), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
