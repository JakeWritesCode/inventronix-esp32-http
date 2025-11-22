#include <Arduino.h>
#include "Inventronix.h"
#include <WiFiClientSecure.h>

// Constructor
Inventronix::Inventronix() {
    _retryAttempts = INVENTRONIX_DEFAULT_RETRY_ATTEMPTS;
    _retryDelay = INVENTRONIX_DEFAULT_RETRY_DELAY;
    _verboseLogging = INVENTRONIX_VERBOSE_LOGGING;
    _debugMode = false;
}

// Initialize the library
void Inventronix::begin(const char* projectId, const char* apiKey) {
    _projectId = String(projectId);  // Convert C-string to Arduino String
    _apiKey = String(apiKey);

    if (_verboseLogging) {
        Serial.println("✅ Inventronix initialized");
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
    // Check WiFi first
    if (!checkWiFi()) {
        return false;
    }

    // Retry loop with exponential backoff
    for (int attempt = 1; attempt <= _retryAttempts; attempt++) {
        String responseBody;
        int statusCode = sendHTTPRequest(jsonPayload, responseBody);

        // Log the response details
        if (_verboseLogging && statusCode > 0) {
            Serial.printf("📡 HTTP %d", statusCode);
            if (responseBody.length() > 0 && responseBody.length() < 100) {
                Serial.print(" - ");
                Serial.print(responseBody);
            }
            Serial.println();
        } else if (_verboseLogging && statusCode <= 0) {
            Serial.printf("❌ Request failed (error code: %d)\n", statusCode);
        }

        // Success! (any 2xx status code)
        if (statusCode >= 200 && statusCode < 300) {
            logSuccess();
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
                Serial.printf("⏳ Retrying in %dms... (attempt %d/%d)\n",
                             delayMs, attempt + 1, _retryAttempts);
            }
            delay(delayMs);
        }
    }

    if (_verboseLogging) {
        Serial.println("❌ Max retry attempts reached. Giving up.");
    }
    return false;
}

// Actual HTTP POST
int Inventronix::sendHTTPRequest(const char* jsonPayload, String& responseBody) {
    HTTPClient http;
    WiFiClientSecure client;  // Use secure client for HTTPS

    // Skip SSL certificate verification (for simplicity)
    // In production, you'd want to verify certificates
    client.setInsecure();

    String url = buildURL();

    if (_debugMode) {
        logDebug("POST " + url);
        logDebug("Payload: " + String(jsonPayload));
    }

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

    if (_debugMode) {
        logDebug("Status: " + String(statusCode));
        logDebug("Response: " + responseBody);
    }

    http.end();
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

// Check WiFi connection
bool Inventronix::checkWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("📡 WiFi not connected!");
        Serial.println("   💡 Add this to your setup():");
        Serial.println();
        Serial.println("   WiFi.begin(\"your-ssid\", \"your-password\");");
        Serial.println("   while (WiFi.status() != WL_CONNECTED) delay(500);");
        Serial.println();
        return false;
    }
    return true;
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
