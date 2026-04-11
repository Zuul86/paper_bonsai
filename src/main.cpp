#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <driver/rtc_io.h>

#include "weather.h"
#include "bonsai.h"
#include "time_sync.h"
#include "waveshare_utils.h"

// Define display class for 7.5" V2 3-Color GxEPD2_750c_Z08
// The Waveshare example code matches the init sequence for the GDEY075Z08 (UC8179 controller).
// Waveshare ESP32 Driver Board pins: CS=15, DC=27, RST=26, BUSY=25
GxEPD2_3C<GxEPD2_750c_GDEY075Z08, GxEPD2_750c_GDEY075Z08::HEIGHT / 2> display(GxEPD2_750c_GDEY075Z08(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

// UI alignment variables. These are initialized in setup() after the display
// is rotated, so we can use the correct width and height.
int FONT_WIDTH = 14; 
int FONT_HEIGHT = 24;
int BONSAI_START_X;
int BONSAI_START_Y;

#define BATTERY_PIN 34 // Change to the actual ADC pin connected to your voltage divider

void drawWeatherPanel(WeatherData data, const String& dateTime, float voltage) {
    display.setTextColor(GxEPD_BLACK);
    
    // Temperature in upper left
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 30);
    display.print(data.temperature, 1);
    display.print(" F");

    Serial.println(data.statusMessage + " F");
    // Log the actual weather data
    Serial.printf("Weather Panel Data - Temp: %.1f F, Humidity: %.1f %%, Status: %s\n", data.temperature, data.humidity, data.statusMessage.c_str());

    // Date and Time in bottom right corner (5px padding)
    display.setFont(NULL); // Use default 6x8 font
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTime, 0, 0, &x1, &y1, &w, &h);
    int dateTimeX = display.width() - w - 5;
    int dateTimeY = display.height() - h - 5;
    display.setCursor(dateTimeX, dateTimeY);
    display.print(dateTime);

    // Voltage in bottom left corner (5px padding)
    String volStr = String(voltage, 2) + " V";
    display.getTextBounds(volStr, 0, 0, &x1, &y1, &w, &h);
    int volX = 5;
    int volY = display.height() - h - 5;
    display.setCursor(volX, volY);
    display.print(volStr);

    // Humidity in upper right (7 chars max "100.0 %")
    display.setFont(&FreeMonoBold12pt7b); // Restore font
    int humX = display.width() - (7 * FONT_WIDTH) - 20;
    display.setCursor(humX, 30);
    display.print(data.humidity, 1);
    display.print(" %");
}

void drawBonsaiPanel() {
    for (int y = 0; y < BONSAI_HEIGHT; y++) {
        for (int x = 0; x < BONSAI_WIDTH; x++) {
            BonsaiCell cell = getBonsaiCell(x, y);
            
            if (cell.ch != ' ') {
                // Determine text color based on cell type
                if (cell.type == TYPE_LEAF) {
                    // Random mix of Red and Black leaves based on cell coordinates to remain consistent per tree
                    bool isRed = (x * 13 + y * 7) % 3 == 0;
                    display.setTextColor(isRed ? GxEPD_RED : GxEPD_BLACK);
                } else {
                    display.setTextColor(GxEPD_BLACK);
                }

                // Adafruit_GFX counts Y position from baseline of the font.
                int drawX = BONSAI_START_X + (x * FONT_WIDTH);
                int drawY = BONSAI_START_Y + (y * FONT_HEIGHT);

                display.setCursor(drawX, drawY);
                display.print(cell.ch);
            }
        }
    }
}

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

    display.setFullWindow();
    display.firstPage();

    int pageCount = 0;
    do {
        pageCount++;
        Serial.printf("Rendering page %d...\n", pageCount);
        display.fillScreen(GxEPD_WHITE);
        
        drawWeatherPanel(weather, dateTime, voltage);
        drawBonsaiPanel();
        
    } while (display.nextPage());
    
    unsigned long end_time = millis();
    Serial.printf("Refresh complete. Rendered %d pages. Took %lu ms.\n", pageCount, end_time - start_time);
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
    // Release the RTC hold on pins that were isolated during deep sleep.
    // Without this, the ESP32 cannot communicate with the display upon waking up!
    waveshare_board_wakeup_pins();

    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n--- Paper Bonsai Booting ---");
    logWakeupReason();

    Serial.println("Setting up sensors and time...");
    setupWeather();
    setupTime();

    // The Waveshare ESP32 Driver board uses alternate SPI pins.
    // We must remap them before initializing the display.
    // SCK=13, MISO=12, MOSI=14, CS=15
    Serial.println("Initializing SPI and Display...");
    SPI.end(); 
    // Pass -1 for the CS pin so the ESP32 hardware SPI doesn't fight GxEPD2 for control of GPIO 15
    SPI.begin(13, 12, 14, -1);

    // Since we now actively cut power to the display during deep sleep (RST LOW), 
    // we must bring the power back online and let it stabilize before initializing.
    pinMode(26, OUTPUT);
    digitalWrite(26, HIGH);
    delay(200); // Give the Waveshare MOSFET and panel capacitors time to stabilize VCC

    // If the device was abruptly powered down during a previous refresh, the e-ink capsules
    // retain a residual DC bias, causing ghosted and washed out images. 
    // We can detect a cold power-on and force a screen wipe to clear it.
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        Serial.println("Cold boot detected. Wiping screen to reset microcapsules...");
        display.init(115200, true, 2, false);
        display.clearScreen(); // Forces a full cycle to pristine white, equalizing charge
    } else {
        display.init(115200, true, 2, false); // Normal init ("true, 2, false" avoids power cut)
    }

    display.setRotation(1); // Landscape
    display.setFont(&FreeMonoBold12pt7b);

    // Initialize UI alignment variables now that the display is set up.
    // This ensures the panel is correctly placed regardless of screen rotation.
    BONSAI_START_X = (display.width() - (BONSAI_WIDTH * FONT_WIDTH)) / 2; // Center horizontally
    BONSAI_START_Y = display.height() - (BONSAI_HEIGHT * FONT_HEIGHT) - 100 + FONT_HEIGHT; // 100px padding from bottom

    Serial.printf("Display initialized. Resolution: %dx%d\n", display.width(), display.height());

    // Initial draw
    Serial.println("Starting initial display update...");
    updateDisplay();

    // Hibernate display to completely power off the panel controller
    Serial.println("Hibernating display...");
    display.hibernate();

    // Isolate E-Ink pins during deep sleep to prevent voltage leakage.
    // ESP32 JTAG pins (13, 14, 15) have default pull-ups during deep sleep. This leaks 
    // voltage into the display board, stopping it from properly discharging and resulting 
    // in dim, washed-out colors on subsequent wakeups.
    SPI.end();
    waveshare_board_sleep_pins();

    // Configure deep sleep for 2 minutes (time in microseconds)
    Serial.println("Going to deep sleep for 2 minutes...");
    Serial.flush(); // Ensure serial messages finish transmitting before CPU stops
    esp_sleep_enable_timer_wakeup(2ULL * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
}
