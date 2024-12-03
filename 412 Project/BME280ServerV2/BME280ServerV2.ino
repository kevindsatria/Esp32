#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <map>
#include <vector>
#include <Arduino_JSON.h>
#include "esp_system.h"

Adafruit_BME280 bme;
WebServer server(80);

// Connecting to an access point
const char* ssid = "Dep";
const char* password = "34181280";

// To track each IP request time
std::map<String, unsigned long> lastRequestTime;

unsigned long previousMillis = 0; // Store the last time the function was executed
const unsigned long interval = 2000; // Interval in milliseconds (2 seconds)

// Struct for storing request log details
struct RequestLog {
    String ipAddress;
    String method;
    String uri;
    unsigned long timeSinceLastRequest;
    String userAgent;
};

std::vector<RequestLog> requestLogs; // Vector to store request logs

// Counter for incoming requests and frequency tracking
std::map<String, unsigned long> requestCount;  // Tracks number of requests per client
unsigned long lastCountResetTime = 0;  // Time when counters were last reset

// Function prototypes
void handleData();
void handleRoot();
void handleLogs();
void logRequest();
void prepareAndSendLogs();

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Initialize BME280 sensor
    if (!bme.begin(0x76)) {
        Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
        while (1); // Stop execution
    }

    Serial.println(F("BME280 sensor initialized successfully."));

    unsigned long startAttemptTime = millis();  // Record the time when we start trying to connect
    unsigned long timeout = 15000;  // 15 seconds timeout

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    char result[80];  // Make sure the buffer is large enough
    snprintf(result, sizeof(result), "Attempting to connect to SSID: %s\n", ssid);
    Serial.println(result);
    Serial.print("Connecting.....");

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttemptTime >= timeout) {
            Serial.println("\nFailed to connect to Wi-Fi after 15 seconds.");
            return;  // Exit setup function if the connection attempt times out
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Set up HTTP server routes
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/logs", handleLogs);
    server.on("/cpudata", handleCpuData);
    server.begin();
    Serial.println("HTTP server started.");
}

void loop() {
    unsigned long currentMillis = millis();
    server.handleClient();

    // Cpu data for benchmarking runs every 2 seconds
    if(currentMillis - previousMillis >= interval){
      previousMillis = currentMillis;
      handleCpuData();
    }
}

void handleCpuData(){
    logRequest();

    // Get the current free heap memory
    unsigned long freeHeap = ESP.getFreeHeap();

    // Create a JSON object with only the freeHeap data
    String json = "{";
    json += "\"freeHeap\":" + String(freeHeap);
    json += "}";

    // Send the data to the Flask server
    WiFiClient client;
    if (client.connect("192.168.1.245", 5000)) {  // Replace with your Flask server's IP address
        client.println("POST /receive_data HTTP/1.1");
        client.println("Host: 192.168.1.245");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(json.length());
        client.println();
        client.print(json);

        client.stop();
    }
    
    // Respond back to the client (optional)
    server.send(200, "application/json", json);
}

// Log client request with details
void logRequest() {
    WiFiClient client = server.client();
    if (client) {
        String clientIP = client.remoteIP().toString();
        String method = (server.method() == HTTP_GET) ? "GET" : "POST";
        String uri = server.uri();
        String userAgent = server.header("User-Agent");
        unsigned long currentTime = millis();

        // Check if the request count should be reset
        if (currentTime - lastCountResetTime >= 60000) {  // Reset every 60 seconds (1 minute)
            requestCount.clear();  // Reset the counts
            lastCountResetTime = currentTime;
        }

        // Update the frequency (request count) for the client
        if (requestCount.find(clientIP) == requestCount.end()) {
            requestCount[clientIP] = 1;  // First request from this client
        } else {
            requestCount[clientIP]++;  // Increment request count
        }

        unsigned long timeSinceLastRequest = 0;
        if (lastRequestTime.find(clientIP) != lastRequestTime.end()) {
            timeSinceLastRequest = currentTime - lastRequestTime[clientIP];
        }

        // Update the last request time for the client
        lastRequestTime[clientIP] = currentTime;

        // Log the request
        RequestLog newLog = {clientIP, method, uri, timeSinceLastRequest, userAgent};
        requestLogs.push_back(newLog);

        // Print for debugging
        Serial.printf("Logged Request: IP=%s, Method=%s, URI=%s, Interval=%lums, User-Agent=%s, Frequency=%lu\n",
                      clientIP.c_str(), method.c_str(), uri.c_str(),
                      timeSinceLastRequest, userAgent.c_str(), requestCount[clientIP]);
    }
}

// Serve root HTML page
void handleRoot() {
    logRequest();

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

        button {
            padding: 10px 20px;
            font-size: 1rem;
            margin-top: 20px;
            cursor: pointer;
            border: none;
            border-radius: 5px;
            transition: background-color 0.3s;
        }

        .start-button {
            background-color: #4CAF50; /* Green */
            color: white;
        }

        .stop-button {
            background-color: #f44336; /* Red */
            color: white;
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
        <button id="toggleButton" class="start-button" onclick="toggleFeed()">Start Live Feed</button>
    </div>
    <script>
        let fetchInterval;
        let isFetching = false;

        function fetchData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').innerText = data.temperature + " F";
                    document.getElementById('humidity').innerText = data.humidity + " %";
                    document.getElementById('pressure').innerText = data.pressure + " hPa";
                })
                .catch(error => console.error("Error fetching data:", error));
        }

        function toggleFeed() {
            const button = document.getElementById('toggleButton');
            if (isFetching) {
                clearInterval(fetchInterval);
                button.innerText = "Start Live Feed";
                button.classList.remove("stop-button");
                button.classList.add("start-button");
            } else {
                fetchInterval = setInterval(fetchData, 1500); // Fetch data every second
                button.innerText = "Stop Live Feed";
                button.classList.remove("start-button");
                button.classList.add("stop-button");
            }
            isFetching = !isFetching;
        }
    </script>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

// Serve sensor data as JSON
void handleData() {
    logRequest();

    float temperatureC = bme.readTemperature();
    float temperatureF = temperatureC * 9.0 / 5.0 + 32.0;
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    // JSON payload
    String json = "{";
    json += "\"temperature\":" + String(temperatureF) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"pressure\":" + String(pressure);
    json += "}";

    // Send to Flask server
    WiFiClient client;
    if (client.connect("192.168.1.245", 5000)) { // Replace 192.168.1.245 with your Flask server's IP
        client.println("POST /send-data HTTP/1.1");
        client.println("Host: 192.168.1.245");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(json.length());
        client.println();
        client.print(json);

        // Read and log the server's response
        String response = "";
        while (client.available()) {
            response += client.readString();
        }
        if (response != "") {
            Serial.println("Server Response: " + response);
        } else {
            Serial.println("No response from server.");
        }
    } else {
        Serial.println("Failed to connect to server.");
    }
    client.stop();

    server.send(200, "application/json", json);
}

// Serve request logs as JSON
void handleLogs() {
    JSONVar jsonLogs;

    for (size_t i = 0; i < requestLogs.size(); i++) {
        JSONVar log;
        log["ipAddress"] = requestLogs[i].ipAddress;
        log["method"] = requestLogs[i].method;
        log["uri"] = requestLogs[i].uri;
        log["timeSinceLastRequest"] = requestLogs[i].timeSinceLastRequest;
        log["userAgent"] = requestLogs[i].userAgent;

        jsonLogs[i] = log;
    }

    String jsonString = JSON.stringify(jsonLogs);
    server.send(200, "application/json", jsonString);
}

// Prepare the request logs to send to the server
void prepareAndSendLogs() {
    String logJson = "[";

    for (int i = 0; i < requestLogs.size(); i++) {
        RequestLog log = requestLogs[i];
        logJson += "{";
        logJson += "\"ip\":\"" + log.ipAddress + "\",";
        logJson += "\"method\":\"" + log.method + "\",";
        logJson += "\"uri\":\"" + log.uri + "\",";
        logJson += "\"timeSinceLastRequest\":" + String(log.timeSinceLastRequest) + ",";
        logJson += "\"userAgent\":\"" + log.userAgent + "\",";
        logJson += "\"requestCount\":" + String(requestCount[log.ipAddress]) + "}";
        
        if (i < requestLogs.size() - 1) logJson += ",";
    }
    
    logJson += "]";

    // Send log data to server
    WiFiClient client;
    if (client.connect("192.168.1.245", 5000)) { // Replace with your server IP
        client.println("POST /store-logs HTTP/1.1");
        client.println("Host: 192.168.1.245");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(logJson.length());
        client.println();
        client.print(logJson);
    }
    client.stop();
}