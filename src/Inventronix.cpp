#include <Arduino.h>
#include "Inventronix.h"

#ifdef INVENTRONIX_PLATFORM_ESP
// Static instance pointer for Ticker callbacks
Inventronix* Inventronix::_instance = nullptr;

// Static callback for Ticker (workaround for no lambda capture support)
void Inventronix::pulseOffCallback(int pulseIndex) {
    if (_instance != nullptr) {
        _instance->handlePulseOff(pulseIndex);
    }
}
#endif

// Constructor
Inventronix::Inventronix() {
    _retryAttempts = INVENTRONIX_DEFAULT_RETRY_ATTEMPTS;
    _retryDelay = INVENTRONIX_DEFAULT_RETRY_DELAY;
    _verboseLogging = INVENTRONIX_VERBOSE_LOGGING;
    _debugMode = false;
    _commandCount = 0;
    _pulseCount = 0;
    _wifiManaged = false;

    // Initialize command registry
    for (int i = 0; i < INVENTRONIX_MAX_COMMANDS; i++) {
        _commands[i].registered = false;
    }
    for (int i = 0; i < INVENTRONIX_MAX_PULSES; i++) {
        _pulses[i].registered = false;
        _pulses[i].active = false;
    }
}

// Initialize the library
void Inventronix::begin(const char* projectId, const char* apiKey) {
    _projectId = String(projectId);  // Convert C-string to Arduino String
    _apiKey = String(apiKey);

#ifdef INVENTRONIX_PLATFORM_ESP
    // Store instance pointer for static Ticker callbacks
    _instance = this;
#endif

    if (_verboseLogging) {
        Serial.println("Inventronix initialized");
        Serial.print("   Project ID: ");
        Serial.println(_projectId);
    }
}

// Set the schema ID (optional)
void Inventronix::setSchemaId(const char* schemaId) {
    _schemaId = String(schemaId);
}

// Set retry attempts
void Inventronix::setRetryAttempts(int attempts) {
    _retryAttempts = attempts;
}

// Set retry delay
void Inventronix::setRetryDelay(int milliseconds) {
    _retryDelay = milliseconds;
}

// Set verbose logging
void Inventronix::setVerboseLogging(bool enabled) {
    _verboseLogging = enabled;
}

// Set debug mode
void Inventronix::setDebugMode(bool enabled) {
    _debugMode = enabled;
}

// Core HTTP POST with retry logic
bool Inventronix::sendPayload(const char* jsonPayload) {
    // Ensure WiFi is connected (auto-reconnect if needed)
    if (!ensureWiFi()) {
        return false;
    }

    // Retry loop with exponential backoff
    for (int attempt = 1; attempt <= _retryAttempts; attempt++) {
        String responseBody;
        int statusCode = sendHTTPRequest(jsonPayload, responseBody);

        // Log the response details
        if (_verboseLogging && statusCode > 0) {
            Serial.print("📡 HTTP ");
            Serial.print(statusCode);
            if (responseBody.length() > 0 && responseBody.length() < 100) {
                Serial.print(" - ");
                Serial.print(responseBody);
            }
            Serial.println();
        } else if (_verboseLogging && statusCode <= 0) {
            Serial.print("❌ Request failed (error code: ");
            Serial.print(statusCode);
            Serial.println(")");
        }

        // Success! (any 2xx status code)
        if (statusCode >= 200 && statusCode < 300) {
            logSuccess();
            processCommands(responseBody);
            return true;
        }

        // Don't retry on client errors (except 429 rate limit)
        if (statusCode >= 400 && statusCode < 500 && statusCode != 429) {
            logError(statusCode, responseBody);
            return false;
        }

        // Retry on 429, 5xx, or network errors
        if (attempt < _retryAttempts) {
            int delayMs = _retryDelay * pow(2, attempt - 1);  // Exponential backoff
            if (delayMs > INVENTRONIX_MAX_RETRY_DELAY) {
                delayMs = INVENTRONIX_MAX_RETRY_DELAY;
            }

            if (_verboseLogging) {
                Serial.print("⏳ Retrying in ");
                Serial.print(delayMs);
                Serial.print("ms... (attempt ");
                Serial.print(attempt + 1);
                Serial.print("/");
                Serial.print(_retryAttempts);
                Serial.println(")");
            }
            delay(delayMs);
        }
    }

    if (_verboseLogging) {
        Serial.println("❌ Max retry attempts reached. Giving up.");
    }
    return false;
}

// Actual HTTP POST - platform-specific implementations
int Inventronix::sendHTTPRequest(const char* jsonPayload, String& responseBody) {
    String url = buildURL();

    if (_debugMode) {
        logDebug("POST " + url);
        logDebug("Payload: " + String(jsonPayload));
    }

#ifdef INVENTRONIX_PLATFORM_ESP
    // ESP32/ESP8266 implementation using HTTPClient
    HTTPClient http;
    WiFiClientSecure client;

    // Skip SSL certificate verification (for simplicity)
    client.setInsecure();

    http.begin(client, url);
    http.setTimeout(INVENTRONIX_HTTP_TIMEOUT);

    // Set headers
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Api-Key", _apiKey);
    http.addHeader("X-Project-Id", _projectId);
    http.addHeader("User-Agent", INVENTRONIX_USER_AGENT);

    // Send POST request
    int statusCode = http.POST(jsonPayload);

    // Get response body
    if (statusCode > 0) {
        responseBody = http.getString();
    }

    http.end();

#else
    // Arduino UNO R4 WiFi / Renesas implementation using ArduinoHttpClient
    // R4's WiFiSSLClient has known issues - must use class member, not local variable
    const char* host = "api.inventronix.club";
    String path = String(INVENTRONIX_INGEST_ENDPOINT);
    if (_schemaId.length() > 0) {
        path += "?schema_id=" + _schemaId;
    }

    // Drain any residual data and reset the client properly
    while (_sslClient.available()) {
        _sslClient.read();
    }
    if (_sslClient.connected()) {
        _sslClient.stop();
    }

    // Explicitly connect first (R4 WiFiSSLClient quirk - helps with some servers)
    if (_debugMode) {
        logDebug("Connecting to " + String(host) + ":443...");
    }

    if (!_sslClient.connect(host, 443)) {
        if (_debugMode) {
            logDebug("SSL connection failed!");
        }
        return -3;  // Connection failed
    }

    if (_debugMode) {
        logDebug("SSL connected, sending request...");
    }

    HttpClient http(_sslClient, host, 443);
    http.setTimeout(INVENTRONIX_HTTP_TIMEOUT);

    http.beginRequest();
    http.post(path);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("X-Api-Key", _apiKey);
    http.sendHeader("X-Project-Id", _projectId);
    http.sendHeader("User-Agent", INVENTRONIX_USER_AGENT);
    http.sendHeader("Content-Length", strlen(jsonPayload));
    http.beginBody();
    http.print(jsonPayload);
    http.endRequest();

    int statusCode = http.responseStatusCode();

    // Get response body
    if (statusCode > 0) {
        responseBody = http.responseBody();
    }

    // Clean up for next request
    http.stop();

#endif

    if (_debugMode) {
        logDebug("Status: " + String(statusCode));
        logDebug("Response: " + responseBody);
    }

    return statusCode;
}

// Error logging with helpful messages
void Inventronix::logError(int statusCode, const String& responseBody) {
    if (!_verboseLogging) return;

    Serial.println();  // Blank line for readability

    switch (statusCode) {
        case 400:
            Serial.println("❌ Schema Validation Failed!");
            Serial.println("   Your data doesn't match the server-side schema");
            Serial.println();
            if (responseBody.length() > 0) {
                Serial.println("   📋 Validation error:");
                Serial.println("   " + responseBody);
                Serial.println();
            }
            Serial.println("   💡 Fix your data or update the schema at:");
            Serial.println("   https://inventronix.club/iot-relay/projects/" + _projectId + "/schemas");
            break;

        case 401:
            Serial.println("🔒 Authentication failed!");
            Serial.println("   Your PROJECT_ID or API_KEY is incorrect");
            Serial.println();
            Serial.println("   💡 Check your credentials at:");
            Serial.println("   https://inventronix.club/iot-relay");
            break;

        case 429:
            Serial.println("⏱️  Rate limit exceeded");
            Serial.println("   Your project allows 6 requests/min");
            Serial.println();
            Serial.println("   💡 Upgrade at:");
            Serial.println("   https://inventronix.club/iot-relay");
            break;

        case 503:
        case 502:
        case 500:
            Serial.println("⚠️  Server error (" + String(statusCode) + ")");
            Serial.println("   This is a temporary issue on our side");
            break;

        default:
            Serial.println("❌ Request failed (HTTP " + String(statusCode) + ")");
            if (responseBody.length() > 0) {
                Serial.println("   Response: " + responseBody);
            }
            break;
    }

    Serial.println();
}

// Success logging
void Inventronix::logSuccess() {
    if (!_verboseLogging) return;

    Serial.println("✅ Data sent successfully!");
    Serial.println("   🌐 View your data: https://inventronix.club/iot-relay/projects/" + _projectId + "/payloads");
    Serial.println();
}

// Debug logging
void Inventronix::logDebug(const String& message) {
    if (_debugMode) {
        Serial.println("🔍 [DEBUG] " + message);
    }
}

// Connect to WiFi with timeout
bool Inventronix::connectWiFi(const char* ssid, const char* password, unsigned long timeoutMs) {
    _wifiSsid = String(ssid);
    _wifiPassword = String(password);
    _wifiManaged = true;

    if (_verboseLogging) {
        Serial.print("Connecting to WiFi");
    }

    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            if (_verboseLogging) {
                Serial.println("\nWiFi connection timed out");
            }
            return false;
        }
        delay(500);
        if (_verboseLogging) {
            Serial.print(".");
        }
    }

    // Wait for DHCP to assign a valid IP (R4 WiFi module is slow)
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        if (millis() - startTime > timeoutMs) {
            if (_verboseLogging) {
                Serial.println("\nDHCP timed out");
            }
            return false;
        }
        delay(100);
    }

    if (_verboseLogging) {
        Serial.println("\nWiFi connected");
        Serial.print("   IP address: ");
        Serial.println(WiFi.localIP());
    }

    return true;
}

// Check if WiFi is connected
bool Inventronix::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Try to reconnect WiFi (shorter timeout for auto-reconnect)
bool Inventronix::tryReconnectWiFi(unsigned long timeoutMs) {
    if (!_wifiManaged || _wifiSsid.length() == 0) {
        return false;  // We don't have credentials
    }

    if (_verboseLogging) {
        Serial.println("WiFi disconnected, reconnecting...");
    }

    WiFi.disconnect();
    delay(100);
    WiFi.begin(_wifiSsid.c_str(), _wifiPassword.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            if (_verboseLogging) {
                Serial.println("Reconnect timed out");
            }
            return false;
        }
        delay(500);
        if (_verboseLogging) {
            Serial.print(".");
        }
    }

    if (_verboseLogging) {
        Serial.println("\nReconnected to WiFi");
    }

    return true;
}

// Ensure WiFi is connected, attempt reconnect if not
bool Inventronix::ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    // Try to reconnect if we have credentials
    if (_wifiManaged) {
        return tryReconnectWiFi();
    }

    // No credentials stored - user is managing WiFi themselves
    if (_verboseLogging) {
        Serial.println("WiFi not connected!");
        Serial.println("   Use inventronix.connectWiFi(ssid, password) or connect manually");
    }
    return false;
}

// Build the API URL with query parameters
String Inventronix::buildURL() {
    String url = String(INVENTRONIX_API_BASE_URL) +
                 String(INVENTRONIX_INGEST_ENDPOINT);

    // Add schema_id as query parameter if set
    if (_schemaId.length() > 0) {
        url += "?schema_id=" + _schemaId;
    }

    return url;
}

// ============================================
// COMMAND HANDLING
// ============================================

// Register a toggle-style command handler
void Inventronix::onCommand(const char* commandName, CommandCallback callback) {
    if (_commandCount >= INVENTRONIX_MAX_COMMANDS) {
        if (_verboseLogging) {
            Serial.println("⚠️  Max commands registered, ignoring: " + String(commandName));
        }
        return;
    }

    _commands[_commandCount].name = String(commandName);
    _commands[_commandCount].callback = callback;
    _commands[_commandCount].registered = true;
    _commandCount++;

    if (_verboseLogging) {
        Serial.println("📝 Registered command: " + String(commandName));
    }
}

// Register a pulse command - simple pin-based version
void Inventronix::onPulse(const char* commandName, int pin, unsigned long durationMs) {
    if (_pulseCount >= INVENTRONIX_MAX_PULSES) {
        if (_verboseLogging) {
            Serial.println("⚠️  Max pulse commands registered, ignoring: " + String(commandName));
        }
        return;
    }

    _pulses[_pulseCount].name = String(commandName);
    _pulses[_pulseCount].pin = pin;
    _pulses[_pulseCount].durationMs = durationMs;  // 0 = pull from args
    _pulses[_pulseCount].onCallback = nullptr;
    _pulses[_pulseCount].offCallback = nullptr;
    _pulses[_pulseCount].active = false;
    _pulses[_pulseCount].registered = true;

    // Ensure pin is configured as output
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    _pulseCount++;

    if (_verboseLogging) {
        Serial.print("📝 Registered pulse command: " + String(commandName));
        Serial.print(" (pin ");
        Serial.print(pin);
        if (durationMs > 0) {
            Serial.print(", ");
            Serial.print(durationMs);
            Serial.print("ms");
        } else {
            Serial.print(", duration from args");
        }
        Serial.println(")");
    }
}

// Register a pulse command - callback-based version
void Inventronix::onPulse(const char* commandName, unsigned long durationMs,
                          PulseOnCallback onCb, PulseOffCallback offCb) {
    if (_pulseCount >= INVENTRONIX_MAX_PULSES) {
        if (_verboseLogging) {
            Serial.println("⚠️  Max pulse commands registered, ignoring: " + String(commandName));
        }
        return;
    }

    _pulses[_pulseCount].name = String(commandName);
    _pulses[_pulseCount].pin = -1;  // Using callbacks, not direct pin
    _pulses[_pulseCount].durationMs = durationMs;
    _pulses[_pulseCount].onCallback = onCb;
    _pulses[_pulseCount].offCallback = offCb;
    _pulses[_pulseCount].active = false;
    _pulses[_pulseCount].registered = true;
    _pulseCount++;

    if (_verboseLogging) {
        Serial.print("📝 Registered pulse command: " + String(commandName));
        Serial.print(" (callback, ");
        if (durationMs > 0) {
            Serial.print(durationMs);
            Serial.print("ms");
        } else {
            Serial.print("duration from args");
        }
        Serial.println(")");
    }
}

// Check if a pulse command is currently active
bool Inventronix::isPulsing(const char* commandName) {
    for (int i = 0; i < _pulseCount; i++) {
        if (_pulses[i].registered && _pulses[i].name == commandName) {
            return _pulses[i].active;
        }
    }
    return false;
}

// Process commands from the ingest response
void Inventronix::processCommands(const String& responseBody) {
    if (responseBody.length() == 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (error) {
        if (_debugMode) {
            logDebug("Failed to parse response JSON: " + String(error.c_str()));
        }
        return;
    }

    // Check for commands array
    if (!doc["commands"].is<JsonArray>()) {
        return;  // No commands, that's fine
    }

    JsonArray commands = doc["commands"].as<JsonArray>();
    int commandCount = commands.size();

    if (commandCount == 0) return;

    if (_verboseLogging) {
        Serial.print("📨 Received ");
        Serial.print(commandCount);
        Serial.println(" command(s)");
    }

    for (JsonObject cmd : commands) {
        const char* command = cmd["command"] | "";
        const char* executionId = cmd["execution_id"] | "";
        JsonObject args = cmd["arguments"].as<JsonObject>();

        if (strlen(command) > 0) {
            dispatchCommand(command, args, executionId);
        }
    }
}

// Dispatch a command to the appropriate handler
void Inventronix::dispatchCommand(const char* command, JsonObject args, const char* executionId) {
    if (_verboseLogging) {
        Serial.print("⚡ Dispatching command: ");
        Serial.println(command);
    }

    // Check toggle commands first
    for (int i = 0; i < _commandCount; i++) {
        if (_commands[i].registered && _commands[i].name == command) {
            if (_debugMode) {
                logDebug("Matched toggle command handler");
            }
            _commands[i].callback(args);

            // TODO: Send ack to server when endpoint exists
            // ackExecution(executionId, true);
            return;
        }
    }

    // Check pulse commands
    for (int i = 0; i < _pulseCount; i++) {
        if (_pulses[i].registered && _pulses[i].name == command) {
            // Ignore if already pulsing (spam protection)
            if (_pulses[i].active) {
                if (_verboseLogging) {
                    Serial.println("   ⏭️  Already pulsing, ignoring");
                }
                return;
            }

            // Determine duration: use registered value, or pull from args
            unsigned long duration = _pulses[i].durationMs;
            if (duration == 0) {
                // Try to get from command arguments
                duration = args["duration"] | args["duration_ms"] | 0UL;
                if (duration == 0) {
                    if (_verboseLogging) {
                        Serial.println("   ❌ No duration specified (set in onPulse or send in args)");
                    }
                    return;
                }
            }

            if (_verboseLogging) {
                Serial.print("🔄 Pulsing for ");
                Serial.print(duration);
                Serial.println("ms");
            }

            // Start the pulse
            _pulses[i].active = true;

            if (_pulses[i].pin >= 0) {
                // Pin-based pulse
                digitalWrite(_pulses[i].pin, HIGH);
            } else if (_pulses[i].onCallback) {
                // Callback-based pulse
                _pulses[i].onCallback();
            }

#ifdef INVENTRONIX_PLATFORM_ESP
            // Schedule the off using Ticker with static callback
            _pulses[i].ticker.once_ms(duration, pulseOffCallback, i);
#else
            // Store timing for millis-based check in loop()
            _pulses[i].startTime = millis();
            _pulses[i].activeDuration = duration;
#endif

            // TODO: Send ack to server when endpoint exists
            // ackExecution(executionId, true);
            return;
        }
    }

    // No handler found
    if (_verboseLogging) {
        Serial.print("   ⚠️  No handler registered for command: ");
        Serial.println(command);
    }
}

// Handle pulse off (called by Ticker)
void Inventronix::handlePulseOff(int pulseIndex) {
    if (pulseIndex < 0 || pulseIndex >= _pulseCount) return;
    if (!_pulses[pulseIndex].active) return;

    if (_verboseLogging) {
        Serial.print("⏹️  Pulse complete: ");
        Serial.println(_pulses[pulseIndex].name);
    }

    if (_pulses[pulseIndex].pin >= 0) {
        // Pin-based - turn off
        digitalWrite(_pulses[pulseIndex].pin, LOW);
    } else if (_pulses[pulseIndex].offCallback) {
        // Callback-based - call off callback
        _pulses[pulseIndex].offCallback();
    }

    _pulses[pulseIndex].active = false;
}

// Loop method - call this in your loop() for pulse timing on non-ESP platforms
void Inventronix::loop() {
#ifndef INVENTRONIX_PLATFORM_ESP
    // Check all active pulses for timeout
    unsigned long now = millis();
    for (int i = 0; i < _pulseCount; i++) {
        if (_pulses[i].registered && _pulses[i].active) {
            if (now - _pulses[i].startTime >= _pulses[i].activeDuration) {
                handlePulseOff(i);
            }
        }
    }
#endif
    // On ESP platforms, Ticker handles timing automatically - loop() is a no-op
}
