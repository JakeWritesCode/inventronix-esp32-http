/**
 * Inventronix Basic Usage Example
 *
 * This example shows how to send data to the Inventronix platform
 * using the core library (without code generation).
 *
 * Supported Hardware:
 * - ESP32 / ESP8266
 * - Arduino UNO R4 WiFi
 *
 * Setup:
 * 1. Install ArduinoJson library (Tools -> Manage Libraries -> Search "ArduinoJson")
 * 2. For Arduino UNO R4 WiFi: Install ArduinoHttpClient library
 * 3. Update WiFi credentials below
 * 4. Update PROJECT_ID and API_KEY from https://inventronix.club/iot-relay/projects
 * 5. Upload to your board
 * 6. Open Serial Monitor (115200 baud) to see output
 */

#include <Inventronix.h>
#include <ArduinoJson.h>

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Inventronix credentials (get these from https://inventronix.club/iot-relay/projects)
#define PROJECT_ID "proj_abc123"
#define API_KEY "key_xyz789"

// Create Inventronix client
Inventronix inventronix;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=================================");
    Serial.println("Inventronix Basic Usage Example");
    Serial.println("=================================\n");

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);

    // Connect to WiFi (with 30s timeout, auto-reconnect on drop)
    if (!inventronix.connectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to WiFi!");
        while (true) delay(1000);  // Halt
    }

    // Optional: Configure retry behavior
    // inventronix.setRetryAttempts(5);
    // inventronix.setRetryDelay(2000);

    // Optional: Enable debug mode to see full HTTP requests/responses
    // inventronix.setDebugMode(true);
}

void loop() {
    // Required for pulse timing on Arduino UNO R4 (no-op on ESP platforms)
    inventronix.loop();

    // Create JSON payload
    JsonDocument doc;

    // Example: Temperature sensor data
    doc["temperature"] = 23.5;
    doc["humidity"] = 65.2;

    // Serialize to string
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("Sending data...");
    Serial.print("Payload: ");
    Serial.println(jsonPayload);
    Serial.println();

    // Send to Inventronix
    bool success = inventronix.sendPayload(jsonPayload.c_str());

    if (success) {
        Serial.println("Successfully sent data!\n");
    } else {
        Serial.println("Failed to send data\n");
    }

    // Wait 60 seconds before sending again
    Serial.println("Waiting 60 seconds...\n");
    delay(60000);
}
