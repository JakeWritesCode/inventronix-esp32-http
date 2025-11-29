#ifndef INVENTRONIX_H
#define INVENTRONIX_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <functional>
#include "InventronixConfig.h"

// Max registered commands (adjust based on memory constraints)
#define INVENTRONIX_MAX_COMMANDS 16
#define INVENTRONIX_MAX_PULSES 8

// Callback types
using CommandCallback = std::function<void(JsonObject args)>;
using PulseOnCallback = std::function<void()>;
using PulseOffCallback = std::function<void()>;

// Command registration entry
struct CommandHandler {
    String name;
    CommandCallback callback;
    bool registered;
};

// Pulse command entry
struct PulseHandler {
    String name;
    int pin;                    // -1 if using callbacks instead
    unsigned long durationMs;   // 0 = pull from command args
    PulseOnCallback onCallback;
    PulseOffCallback offCallback;
    Ticker ticker;
    bool active;                // Currently pulsing?
    bool registered;
};

class Inventronix {
public:
    Inventronix();

    // Setup methods
    void begin(const char* projectId, const char* apiKey);
    void setSchemaId(const char* schemaId);

    // Core functionality
    bool sendPayload(const char* jsonPayload);

    // Command registration - toggle style
    void onCommand(const char* commandName, CommandCallback callback);

    // Pulse registration - pin-based (simple)
    void onPulse(const char* commandName, int pin, unsigned long durationMs = 0);

    // Pulse registration - callback-based (flexible)
    void onPulse(const char* commandName, unsigned long durationMs,
                 PulseOnCallback onCb, PulseOffCallback offCb);

    // Pulse status helpers
    bool isPulsing(const char* commandName);

    // Configuration
    void setRetryAttempts(int attempts);
    void setRetryDelay(int milliseconds);
    void setVerboseLogging(bool enabled);
    void setDebugMode(bool enabled);

private:
    // Member variables
    String _projectId;
    String _apiKey;
    String _schemaId;
    int _retryAttempts;
    int _retryDelay;
    bool _verboseLogging;
    bool _debugMode;

    // Command registry
    CommandHandler _commands[INVENTRONIX_MAX_COMMANDS];
    int _commandCount;

    // Pulse registry
    PulseHandler _pulses[INVENTRONIX_MAX_PULSES];
    int _pulseCount;

    // Private helper methods
    String buildURL();
    void logError(int statusCode, const String& responseBody);
    void logSuccess();
    void logDebug(const String& message);
    bool checkWiFi();
    int sendHTTPRequest(const char* jsonPayload, String& responseBody);

    // Command processing
    void processCommands(const String& responseBody);
    void dispatchCommand(const char* command, JsonObject args, const char* executionId);
    void handlePulseOff(int pulseIndex);

    // Static callback for Ticker (ESP32 Ticker doesn't support lambdas with captures)
    static Inventronix* _instance;
    static void pulseOffCallback(int pulseIndex);
};

#endif
