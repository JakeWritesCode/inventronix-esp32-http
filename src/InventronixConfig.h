#ifndef INVENTRONIX_CONFIG_H
#define INVENTRONIX_CONFIG_H

// API Configuration
#define INVENTRONIX_API_BASE_URL "https://api.inventronix.club"
#define INVENTRONIX_INGEST_ENDPOINT "/v1/iot/ingest"

// Retry Configuration
#define INVENTRONIX_DEFAULT_RETRY_ATTEMPTS 3
#define INVENTRONIX_DEFAULT_RETRY_DELAY 1000  // milliseconds
#define INVENTRONIX_MAX_RETRY_DELAY 10000     // 10 seconds max

// Request Configuration
#define INVENTRONIX_HTTP_TIMEOUT 10000  // 10 second timeout
#define INVENTRONIX_USER_AGENT "Inventronix-Arduino/1.0.0 (ESP32-C3)"

// Logging
#define INVENTRONIX_VERBOSE_LOGGING true

#endif
