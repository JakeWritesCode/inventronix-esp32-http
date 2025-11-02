#ifndef INVENTRONIX_H
#define INVENTRONIX_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "InventronixConfig.h"

class Inventronix {
public:
    // Constructor (called when object is created)
    Inventronix();

    // Setup methods
    void begin(const char* projectId, const char* apiKey);
    void setSchemaId(const char* schemaId);

    // Core functionality
    bool sendPayload(const char* jsonPayload);

    // Configuration
    void setRetryAttempts(int attempts);
    void setRetryDelay(int milliseconds);
    void setVerboseLogging(bool enabled);
    void setDebugMode(bool enabled);

private:
    // Member variables (stored in the object)
    String _projectId;
    String _apiKey;
    String _schemaId;
    int _retryAttempts;
    int _retryDelay;
    bool _verboseLogging;
    bool _debugMode;

    // Private helper methods
    String buildURL();
    void logError(int statusCode, const String& responseBody);
    void logSuccess();
    void logDebug(const String& message);
    bool checkWiFi();
    int sendHTTPRequest(const char* jsonPayload, String& responseBody);
};

#endif
