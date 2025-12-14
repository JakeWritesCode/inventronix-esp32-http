/**
 * Inventronix Basic Usage Example
 *
 * This example shows how to send data to the Inventronix platform
 * using the core library (without code generation).
 *
 * Hardware: Inventronix ESP32-C3 board
 *
 * Setup:
 * 1. Install ArduinoJson library (Tools -> Manage Libraries -> Search "ArduinoJson")
 * 2. Update WiFi credentials below
 * 3. Update PROJECT_ID and API_KEY from https://inventronix.club/iot-relay/projects
 * 4. Upload to your board
 * 5. Open Serial Monitor (115200 baud) to see output
 */

#include <WiFi.h>
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
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);  // Wait for serial monitor to connect

    Serial.println("\n\n=================================");
    Serial.println("Inventronix Basic Usage Example");
    Serial.println("=================================\n");

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("   IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);

    // Optional: Configure retry behavior
    // inventronix.setRetryAttempts(5);
    // inventronix.setRetryDelay(2000);

    // Optional: Enable debug mode to see full HTTP requests/responses
    // inventronix.setDebugMode(true);
}

void loop() {
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
