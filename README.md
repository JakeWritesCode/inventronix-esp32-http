# Inventronix Arduino Library

Dead simple IoT data ingestion for ESP32-C3 boards.

## Features

- Simple, beginner-friendly API
- Automatic retry logic with exponential backoff
- Helpful error messages with debugging tips
- Built-in WiFi connection checking
- Support for schema-based validation
- Type-safe code generation support (coming soon)

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

// Inventronix credentials (from https://inventronix.io/projects)
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
    StaticJsonDocument<200> doc;
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

See the `examples/BasicUsage/BasicUsage.ino` file for a complete working example.

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
Verify your PROJECT_ID and API_KEY are correct. Get them from: https://inventronix.io/projects

### Upload fails
- Check USB cable connection
- Try holding the BOOT button while uploading
- Verify correct board selected in Arduino IDE

## Memory Usage

The library is designed for embedded systems with limited memory:

- Uses efficient C-style strings for function parameters
- Minimal heap allocation
- Typical RAM usage: ~2-3KB

## License

MIT License - see LICENSE file for details

## Support

- Documentation: https://docs.inventronix.io
- Issues: https://github.com/inventronix/inventronix-arduino/issues
- Email: support@inventronix.io

## Contributing

Contributions are welcome! Please open an issue or pull request on GitHub.
