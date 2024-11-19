#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <map>  // For tracking request frequencies
#include <ctime>  // For timestamps

Adafruit_BME280 bme;
WebServer server(80);

const char* ssid = "Dep";
const char* password = "34181280";

// Map to track request frequencies and timestamps by IP
std::map<String, std::vector<unsigned long>> requestFrequencies;  // Tracks request timestamps per IP
std::map<String, bool> ipStatus;  // Tracks if an IP is currently marked as malicious

// Constants for request tracking
const unsigned long TIME_THRESHOLD = 1000; // 2 seconds to track frequency
const size_t MAX_REQUESTS = 200; // Max requests per TIME_THRESHOLD considered suspicious

void handleData();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize BME280 sensor
  if (!bme.begin(0x76)) {  // Use 0x76 address directly
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1);  // Loop forever if sensor is not found
  }

  Serial.println(F("BME280 sensor is connected successfully."));

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the HTTP server and route handlers
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // Handles incoming client requests
  logRequest();
}

// Logs client IP address, request frequency, and timestamp
void logRequest() {
  WiFiClient client = server.client();
  if (client) {
    String clientIP = client.remoteIP().toString();
    unsigned long currentTime = millis(); // Current time in milliseconds
    size_t requestSize = client.available();

    // Log data in CSV format
    Serial.print(clientIP);
    Serial.print(",");
    Serial.print(currentTime);
    Serial.print(",");
    Serial.print(requestSize);
    
    // Flush the buffer to make sure data is written
    Serial.flush();

    // Track request frequency for this IP
    if (requestFrequencies.find(clientIP) == requestFrequencies.end()) {
      requestFrequencies[clientIP] = std::vector<unsigned long>();
      ipStatus[clientIP] = false;  // Mark as normal initially
    }

    auto& timestamps = requestFrequencies[clientIP];
    timestamps.push_back(currentTime);

    // Remove requests older than the TIME_THRESHOLD
    while (timestamps.size() > 0 && currentTime - timestamps.front() > TIME_THRESHOLD) {
      timestamps.erase(timestamps.begin());
    }

    // Count the number of requests within the threshold
    int frequency = timestamps.size();
    if (frequency > MAX_REQUESTS) {
      if (!ipStatus[clientIP]) {  // Only mark as malicious if not already flagged
        ipStatus[clientIP] = true;
        Serial.print(",Malicious");  // Mark as suspicious if requests exceed threshold
      } else {
        Serial.print(",Malicious (Persisted)");  // Still malicious
      }
    } else {
      if (ipStatus[clientIP]) {  // If previously flagged as malicious, reset after falling below threshold
        ipStatus[clientIP] = false;  // Reset status to normal
        Serial.print(",Normal (Recovered)");  // Reset to normal after dropping below threshold
      } else {
        Serial.print(",Normal");  // Mark as normal otherwise
      }
    }

    Serial.println();  // Move to the next line

    // Flush after printing the full log
    Serial.flush();
  }
}

// Handles the root page, serving the HTML
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css2?family=Comfortaa:wght@300;400;700&display=swap" rel="stylesheet">
    <title>BME280 Readings</title>
    <style>
        * {
            box-sizing: border-box;
            font-family: 'Comfortaa', sans-serif;
            overflow-y: hidden;
        }

        html, body {
            width: auto;
            height: 100vh;
            margin: 0;
            text-align: center;
            background: linear-gradient(135deg, rgb(135, 169, 172), rgb(207, 212, 178));
        }

        body {
            display: flex;
            justify-content: center;
            align-items: center;
        }

        h1 {
            margin-bottom: 10%;
            font-size: 2rem;
        }

        h3 {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 20% 0;
        }

        .data {
            background-color: rgba(255, 255, 255, 0.589);
            padding: 20px;
            border-radius: 10px;
            border: solid 2px black;
        }

        span {
            font-size: 150%;
        }
    </style>
</head>
<body>
    <div>
        <h1>BME280 Sensor Data</h1>
        <div class="data">
            <h3>Temperature: <span id="temperature">F</span></h3>
            <h3>Humidity: <span id="humidity">%</span></h3>
            <h3>Pressure: <span id="pressure">hPa</span></h3>
        </div>
    </div>
    <script>
        function fetchData() {
            fetch('/data').then(response => response.json()).then(data => {
                document.getElementById('temperature').innerText = `${data.temperature} F`;
                document.getElementById('humidity').innerText = `${data.humidity} %`;
                document.getElementById('pressure').innerText = `${data.pressure} hPa`;
            });
        }
        setInterval(fetchData, 2000);  // Fetch data every 2 seconds
    </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// Handles the `/data` endpoint, sending JSON sensor data
void handleData() {
  float temperatureC = bme.readTemperature();
  float temperatureF = temperatureC * 9.0 / 5.0 + 32.0;
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  String json = "{";
  json += "\"temperature\":" + String(temperatureF) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"pressure\":" + String(pressure);
  json += "}";

  server.send(200, "application/json", json);
}
