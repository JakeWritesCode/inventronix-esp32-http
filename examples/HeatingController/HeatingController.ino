/**
 * Heating Controller Example
 *
 * Demonstrates:
 * - Toggle commands (heater on/off)
 * - Reporting actual hardware state back to server
 * - Using DHT11 temperature/humidity sensor
 *
 * Hardware:
 * - ESP32-C3 or similar
 * - DHT11 sensor on pin 4
 * - Relay/heater control on pin 7
 *
 * Setup in Inventronix Connect:
 * 1. Create a schema with: temperature (number), humidity (number), heater_on (boolean)
 * 2. Create actions: "Turn Heater On" (device_command: heater_on), "Turn Heater Off" (device_command: heater_off)
 * 3. Create rules:
 *    - If avg(temperature) in 5m < 18 AND last(heater_on) == false -> Turn Heater On
 *    - If avg(temperature) in 5m > 22 AND last(heater_on) == true -> Turn Heater Off
 */

#include <WiFi.h>
#include <Inventronix.h>
#include <ArduinoJson.h>
#include "DHT.h"

// WiFi credentials - CHANGE THESE
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Inventronix credentials - get these from your project settings
#define PROJECT_ID "your-project-id"
#define API_KEY "your-api-key"

// Pin definitions
#define HEATER_PIN 7
#define DHT_PIN 4

// Create instances
Inventronix inventronix;
DHT dht(DHT_PIN, DHT11);

void setup() {
    Serial.begin(115200);
    delay(1000);

    dht.begin();
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(HEATER_PIN, LOW);

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);

    // Register command handlers
    inventronix.onCommand("heater_on", [](JsonObject args) {
        Serial.println("Heater ON");
        digitalWrite(HEATER_PIN, HIGH);
    });

    inventronix.onCommand("heater_off", [](JsonObject args) {
        Serial.println("Heater OFF");
        digitalWrite(HEATER_PIN, LOW);
    });
}

void loop() {
    // Read sensors
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("DHT read failed, skipping...");
        delay(2000);
        return;
    }

    // Build payload - report ACTUAL hardware state
    JsonDocument doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["heater_on"] = (digitalRead(HEATER_PIN) == HIGH);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.print("Sending: ");
    Serial.println(jsonPayload);

    // Send payload - commands are automatically dispatched to handlers
    bool success = inventronix.sendPayload(jsonPayload.c_str());

    if (success) {
        Serial.println("Data sent successfully\n");
    } else {
        Serial.println("Failed to send data\n");
    }

    // 10 second loop
    delay(10000);
}
