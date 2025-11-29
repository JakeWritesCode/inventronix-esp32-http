/**
 * Hydroponic Controller Example
 *
 * Demonstrates:
 * - Toggle commands (heater on/off)
 * - Pulse commands (nutrient pump)
 * - Reporting actual state back to server
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Inventronix.h>
#include <ArduinoJson.h>
#include "DHT.h"

// WiFi credentials
#define WIFI_SSID "26 The Maltings"
#define WIFI_PASSWORD "BrunoTheDog"

// Inventronix credentials
#define PROJECT_ID "e139eeb2-09aa-489a-96df-34d8465fdb3e"
#define API_KEY "1a6b2728-32a5-4905-89a8-674e8de9901b"

// Pin definitions
#define HEATER_PIN 3
#define PUMP_PIN 5
#define DHT_PIN 4

// Timing
#define PUMP_PULSE_MS 5000  // 5 second pump pulse

// Create instances
Inventronix inventronix;
DHT dht11(DHT_PIN, DHT11);

// Actual hardware state (reported in payloads)
int heaterState = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    dht11.begin();
    inventronix.setVerboseLogging(true);

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

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);

    // =========================================
    // REGISTER COMMAND HANDLERS
    // =========================================

    // Toggle command: heater_on
    // Rule example: "If avg temp < 18 last 5 mins AND heater_on == 0, turn heater on"
    inventronix.onCommand("heater_on", [](JsonObject args) {
        Serial.println("🔥 Heater ON");
        digitalWrite(HEATER_PIN, HIGH);
        heaterState = 1;
    });

    // Toggle command: heater_off
    inventronix.onCommand("heater_off", [](JsonObject args) {
        Serial.println("❄️ Heater OFF");
        digitalWrite(HEATER_PIN, LOW);
        heaterState = 0;
    });

    // Pulse command: pump_nutrients
    // Rule example: "If avg EC < 1200 last 30 mins, pump nutrients"
    // Duration hardcoded here - server just sends "pump_nutrients", we handle timing
    inventronix.onPulse("pump_nutrients", PUMP_PIN, PUMP_PULSE_MS);

    // Alternative: duration from server args
    // inventronix.onPulse("pump_nutrients", PUMP_PIN);  // pulls "duration" from command args

    // Alternative: custom callbacks for complex logic
    // inventronix.onPulse("pump_nutrients", PUMP_PULSE_MS,
    //     []() {
    //         Serial.println("💧 Pump starting");
    //         digitalWrite(PUMP_PIN, HIGH);
    //     },
    //     []() {
    //         Serial.println("💧 Pump stopping");
    //         digitalWrite(PUMP_PIN, LOW);
    //     }
    // );

    // Setup pins
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(HEATER_PIN, LOW);
}

void loop() {
    // Read sensors
    float humidity = dht11.readHumidity();
    float temperature = dht11.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("DHT read failed, skipping...");
        delay(2000);
        return;
    }

    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print("°C  Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    // Build payload - report ACTUAL hardware state
    JsonDocument doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["heater_on"] = heaterState;
    doc["pump_on"] = inventronix.isPulsing("pump_nutrients") ? 1 : 0;

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

    // 10 second loop - adjust based on your rate limit
    delay(10000);
}
