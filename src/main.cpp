#include <Arduino.h>
#include <driver/rtc_io.h>

#include "weather.h"
#include "bonsai.h"
#include "time_sync.h"
#include "epaper_driver.h"
#include "bonsai_renderer.h"

#define BATTERY_PIN 34 // Change to the actual ADC pin connected to your voltage divider

EPaperDriver epaper;
BonsaiRenderer renderer(epaper.getDisplay());

void updateDisplay() {
    Serial.println("Refreshing E-Ink Display...");
    unsigned long start_time = millis();
    
    // Generate bonsai once per draw cycle, BEFORE the paging loop
    generateBonsai();
    
    // Fetch weather data once BEFORE the paging loop
    WeatherData weather = getWeatherData();
    String dateTime = getDateTimeString();
    
    // Read battery voltage (Assumes a 1/2 voltage divider, adjust multiplier as needed)
    // ESP32 ADC is 12-bit (0-4095) and reference is ~3.3V
    float voltage = analogRead(BATTERY_PIN) * (3.3 / 4095.0) * 2.0;

    Serial.printf("Data fetched. Temp: %.1f, Time: %s, Voltage: %.2f\n", weather.temperature, dateTime.c_str(), voltage);

    auto& display = epaper.getDisplay();
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        
        renderer.drawWeatherPanel(weather, dateTime, voltage);
        renderer.drawBonsaiPanel();
        
    } while (display.nextPage());
    
    unsigned long end_time = millis();
    Serial.printf("Refresh complete. Took %lu ms.\n", end_time - start_time);
}

void logWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup Reason: Timer (Normal Interval)"); break;
        case ESP_SLEEP_WAKEUP_EXT0:  Serial.println("Wakeup Reason: External Signal (RTC_IO)"); break;
        case ESP_SLEEP_WAKEUP_EXT1:  Serial.println("Wakeup Reason: External Signal (RTC_CNTL)"); break;
        default:                     Serial.printf("Wakeup Reason: Hard Boot / Reset (Code: %d)\n", wakeup_reason); break;
    }
}

void setup() {
    epaper.wakeHardware();

    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n--- Paper Bonsai Booting ---");
    logWakeupReason();

    Serial.println("Setting up sensors and time...");
    setupWeather();
    setupTime();

    Serial.println("Initializing SPI and Display...");
    epaper.initDisplay();
    renderer.initDimensions();

    Serial.printf("Display initialized. Resolution: %dx%d\n", epaper.getDisplay().width(), epaper.getDisplay().height());

    // Initial draw
    Serial.println("Starting initial display update...");
    updateDisplay();

    Serial.println("Hibernating display...");
    epaper.sleep();

    // Configure deep sleep for 2 minutes (time in microseconds)
    Serial.println("Going to deep sleep for 2 minutes...");
    Serial.flush(); // Ensure serial messages finish transmitting before CPU stops
    esp_sleep_enable_timer_wakeup(2ULL * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
}
