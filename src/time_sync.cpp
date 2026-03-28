#include "time_sync.h"
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include "secrets.h"

// Store initialization state in RTC memory so it survives deep sleep
RTC_DATA_ATTR bool timeInitialized = false;

// Timezone configuration (Configure these to match your location)
const long  gmtOffset_sec = -18000;      // Example: -18000 for EST (UTC-5)
const int   daylightOffset_sec = 3600;   // 3600 for 1 hour DST offset (0 if no DST)
const char* ntpServer = "pool.ntp.org";

void setupTime() {
    if (!timeInitialized) {
        Serial.print("Connecting to WiFi to sync time...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
            delay(500);
            Serial.print(".");
            retries++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                timeInitialized = true;
                Serial.println("Time successfully set from NTP.");
            }
        }
        
        // Disconnect and completely disable WiFi radio to save power
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("Time retained from deep sleep.");
    }
}

String getDateTimeString() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%m/%d/%y %H:%M", &timeinfo);
    return String(buffer);
}