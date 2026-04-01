#include "time_sync.h"
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include "secrets.h"

// Store initialization state in RTC memory so it survives deep sleep
RTC_DATA_ATTR bool timeInitialized = false;

// Timezone configuration using POSIX TZ string for Central Time (CST/CDT)
const char* time_zone = "CST6CDT,M3.2.0,M11.1.0";
const char* ntpServer = "pool.ntp.org";

void setupTime() {
    // Always set the timezone, as environment variables are lost during deep sleep
    setenv("TZ", time_zone, 1);
    tzset();

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
            configTzTime(time_zone, ntpServer);
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
    strftime(buffer, sizeof(buffer), "%m/%d/%y %I:%M %p", &timeinfo);
    return String(buffer);
}