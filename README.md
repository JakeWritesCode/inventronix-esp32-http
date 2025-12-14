# Inventronix Arduino Library

Dead simple IoT data ingestion and device control for ESP32-C3 boards.

## Features

- Simple, beginner-friendly API
- **Command handling** - Receive and execute commands from the cloud
- **Pulse commands** - Non-blocking timed actions (pumps, buzzers, etc.)
- Automatic retry logic with exponential backoff
- Helpful error messages with debugging tips
- Built-in WiFi connection checking
- Support for schema-based validation

## Hardware Requirements

- Inventronix ESP32-C3 board
- USB cable for programming
- WiFi network

## Software Requirements

- Arduino IDE 1.8.x or later, OR
- PlatformIO

## Installation

### Arduino IDE

1. Download this library as a ZIP file
2. In Arduino IDE: Sketch > Include Library > Add .ZIP Library
3. Select the downloaded ZIP file
4. Install the ArduinoJson library:
   - Tools > Manage Libraries
   - Search for "ArduinoJson"
   - Install version 6.x or later

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
```

## Quick Start

```cpp
#include <WiFi.h>
#include <Inventronix.h>
#include <ArduinoJson.h>

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Inventronix credentials (from https://inventronix.club/iot-relay/projects)
#define PROJECT_ID "proj_abc123"
#define API_KEY "key_xyz789"

Inventronix inventronix;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Initialize Inventronix
    inventronix.begin(PROJECT_ID, API_KEY);
}

void loop() {
    // Create JSON payload
    JsonDocument doc;
    doc["temperature"] = 23.5;
    doc["humidity"] = 65.2;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Send to Inventronix
    bool success = inventronix.sendPayload(jsonPayload.c_str());

    if (success) {
        Serial.println("Data sent!");
    }

    delay(60000);  // Wait 1 minute
}
```

## Receiving Commands

Commands are triggered by rules you configure in the Inventronix dashboard. When conditions are met (e.g., "temperature > 30"), the server queues commands that your device receives on the next `sendPayload()` call.

### Toggle Commands

For on/off controls like heaters, lights, or relays:

```cpp
int heaterState = 0;  // Track actual hardware state

void setup() {
    // ... WiFi setup ...
    inventronix.begin(PROJECT_ID, API_KEY);

    // Register command handlers
    inventronix.onCommand("heater_on", [](JsonObject args) {
        digitalWrite(HEATER_PIN, HIGH);
        heaterState = 1;
    });

    inventronix.onCommand("heater_off", [](JsonObject args) {
        digitalWrite(HEATER_PIN, LOW);
        heaterState = 0;
    });
}

void loop() {
    JsonDocument doc;
    doc["temperature"] = readTemp();
    doc["heater_on"] = heaterState;  // Report actual state

    String json;
    serializeJson(doc, json);
    inventronix.sendPayload(json.c_str());  // Commands auto-dispatch

    delay(10000);
}
```

### Pulse Commands

For momentary actions like pumps, buzzers, or motors that need to run for a set duration then stop automatically. **Non-blocking** - uses hardware timers so your loop keeps running.

```cpp
#define PUMP_PIN 5

void setup() {
    // ... WiFi setup ...
    inventronix.begin(PROJECT_ID, API_KEY);

    // Simple: pulse pin HIGH for 5 seconds
    inventronix.onPulse("pump_nutrients", PUMP_PIN, 5000);
}

void loop() {
    JsonDocument doc;
    doc["ec_level"] = readEC();
    doc["pump_on"] = inventronix.isPulsing("pump_nutrients") ? 1 : 0;

    String json;
    serializeJson(doc, json);
    inventronix.sendPayload(json.c_str());

    delay(10000);
}
```

**Duration from server:** Omit the duration to pull it from the command's `arguments.duration`:

```cpp
// Server sends: {"command": "pump_nutrients", "arguments": {"duration": 3000}}
inventronix.onPulse("pump_nutrients", PUMP_PIN);  // Uses args.duration
```

**Custom callbacks:** For complex logic:

```cpp
inventronix.onPulse("dispense", 5000,
    []() { digitalWrite(PUMP_PIN, HIGH); Serial.println("Pump ON"); },
    []() { digitalWrite(PUMP_PIN, LOW); Serial.println("Pump OFF"); }
);
```

### Spam Protection

If a pulse command fires while already pulsing, it's ignored. This prevents issues when rules trigger faster than the action completes.

## API Reference

### Constructor

```cpp
Inventronix inventronix;
```

Creates a new Inventronix client instance.

### begin()

```cpp
void begin(const char* projectId, const char* apiKey)
```

Initialize the library with your project credentials.

**Parameters:**
- `projectId`: Your Inventronix project ID (from dashboard)
- `apiKey`: Your API key (from project settings)

**Example:**
```cpp
inventronix.begin("proj_abc123", "key_xyz789");
```

### sendPayload()

```cpp
bool sendPayload(const char* jsonPayload)
```

Send JSON data to the Inventronix platform.

**Parameters:**
- `jsonPayload`: JSON string to send

**Returns:**
- `true` if successful
- `false` if failed (error details printed to Serial)

**Example:**
```cpp
bool success = inventronix.sendPayload("{\"temp\":23.5}");
```

### setSchemaId()

```cpp
void setSchemaId(const char* schemaId)
```

Set the schema ID for validation (optional).

**Parameters:**
- `schemaId`: Schema ID from your Inventronix project

**Example:**
```cpp
inventronix.setSchemaId("schema_xyz");
```

### onCommand()

```cpp
void onCommand(const char* commandName, CommandCallback callback)
```

Register a handler for toggle-style commands.

**Parameters:**
- `commandName`: The command name to listen for (matches `command` field from server)
- `callback`: Function called when command received, signature: `void(JsonObject args)`

**Example:**
```cpp
inventronix.onCommand("light_on", [](JsonObject args) {
    int brightness = args["brightness"] | 100;  // Default 100 if not provided
    analogWrite(LED_PIN, brightness);
});
```

### onPulse() - Pin-based

```cpp
void onPulse(const char* commandName, int pin, unsigned long durationMs = 0)
```

Register a pulse command that toggles a pin HIGH for a duration, then LOW.

**Parameters:**
- `commandName`: The command name to listen for
- `pin`: GPIO pin to pulse (automatically configured as OUTPUT)
- `durationMs`: Pulse duration in milliseconds. If 0, pulls from `args.duration`

**Example:**
```cpp
inventronix.onPulse("buzzer_alert", BUZZER_PIN, 1000);  // 1 second beep
```

### onPulse() - Callback-based

```cpp
void onPulse(const char* commandName, unsigned long durationMs,
             PulseOnCallback onCb, PulseOffCallback offCb)
```

Register a pulse command with custom on/off callbacks.

**Parameters:**
- `commandName`: The command name to listen for
- `durationMs`: Duration between on and off callbacks. If 0, pulls from `args.duration`
- `onCb`: Function called when pulse starts
- `offCb`: Function called when pulse ends

**Example:**
```cpp
inventronix.onPulse("motor_jog", 2000,
    []() { motor.forward(); },
    []() { motor.stop(); }
);
```

### isPulsing()

```cpp
bool isPulsing(const char* commandName)
```

Check if a pulse command is currently active.

**Parameters:**
- `commandName`: The pulse command name to check

**Returns:**
- `true` if the pulse is currently running
- `false` if not pulsing or command not found

**Example:**
```cpp
doc["pump_active"] = inventronix.isPulsing("pump_nutrients") ? 1 : 0;
```

### Configuration Methods

```cpp
void setRetryAttempts(int attempts)
```

Set the number of retry attempts (default: 3).

```cpp
void setRetryDelay(int milliseconds)
```

Set the initial retry delay in milliseconds (default: 1000).

```cpp
void setVerboseLogging(bool enabled)
```

Enable/disable verbose logging (default: true).

```cpp
void setDebugMode(bool enabled)
```

Enable/disable debug mode with full HTTP request/response logging (default: false).

## Error Handling

The library provides helpful error messages for common issues:

### 400 Bad Request
Your data doesn't match the schema. Check your schema definition at the provided URL.

### 401 Unauthorized
Your PROJECT_ID or API_KEY is incorrect. Double-check your credentials.

### 429 Rate Limit
You've exceeded the rate limit. The library will automatically retry with exponential backoff.

### 500/502/503 Server Errors
Temporary server issues. The library will automatically retry.

## Examples

See `src/main.cpp` for a complete hydroponic controller example with:
- Temperature/humidity monitoring
- Toggle commands (heater on/off)
- Pulse commands (nutrient pump)

## Troubleshooting

### "WiFi not connected" error
Make sure you call `WiFi.begin()` and wait for connection before sending data:

```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
}
```

### "Authentication failed" error
Verify your PROJECT_ID and API_KEY are correct. Get them from: https://inventronix.club/iot-relay/projects

### Upload fails
- Check USB cable connection
- Try holding the BOOT button while uploading
- Verify correct board selected in Arduino IDE

## Memory Usage

The library is designed for embedded systems with limited memory:

- Uses efficient C-style strings for function parameters
- Minimal heap allocation
- Up to 16 toggle commands + 8 pulse commands (configurable in `Inventronix.h`)
- Typical usage: ~12% RAM, ~68% Flash on ESP32-C3

## License

MIT License - see LICENSE file for details

## Support

- Documentation: https://docs.inventronix.io
- Issues: https://github.com/inventronix/inventronix-arduino/issues
- Email: support@inventronix.io

## Contributing

Contributions are welcome! Please open an issue or pull request on GitHub.
