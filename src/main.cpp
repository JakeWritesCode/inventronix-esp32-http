/**
 * Main application file for testing the Inventronix library
 * This is a simple test sketch to verify the library compiles correctly
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Inventronix.h>
#include <ArduinoJson.h>

// WiFi credentials (update these for actual testing)
#define WIFI_SSID "26 The Maltings"
#define WIFI_PASSWORD "BrunoTheDog"

// Inventronix credentials
#define PROJECT_ID "748594a3-c402-4c46-9b76-2412983f640d"
#define API_KEY "e2d5427c-917f-45f3-acb1-58cd0175dc07"

// Create Inventronix client
Inventronix inventronix;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);

    // Turn on verbose logging to help with debugging!
    inventronix.setVerboseLogging(true);

    Serial.println("\n\n=================================");
    Serial.println("Inventronix Library Test");
    Serial.println("=================================\n");

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("   IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed");
    }
    Serial.println();

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);
}

void loop() {
    // Create JSON payload
    JsonDocument doc;

    // Example sensor data
    doc["temperature"] = 23.5 + (random(0, 20) / 10.0);
    doc["some_boolean"] = true;
    doc["a_string"] = "toast";


    // Serialize to string
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("Sending data...");
    Serial.print("Payload: ");
    Serial.println(jsonPayload);
    Serial.println();

    // Send to Inventronix (will fail without valid credentials)
    bool success = inventronix.sendPayload(jsonPayload.c_str());

    if (success) {
        Serial.println("Successfully sent data!\n");
    } else {
        Serial.println("Failed to send data (expected without valid WiFi/credentials)\n");
    }

    // Wait 60 seconds
    Serial.println("Waiting 10 seconds...\n");
    delay(10000);
}
