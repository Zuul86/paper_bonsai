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

void drawWeatherPanel(WeatherData data, const String& dateTime) {
    display.setTextColor(GxEPD_BLACK);
    
    // Temperature in upper left
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 30);
    display.print(data.temperature, 1);
    display.print(" C");

    // Date and Time in bottom right corner (5px padding)
    display.setFont(NULL); // Use default 6x8 font
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTime, 0, 0, &x1, &y1, &w, &h);
    int dateTimeX = display.width() - w - 5;
    int dateTimeY = display.height() - h - 5;
    display.setCursor(dateTimeX, dateTimeY);
    display.print(dateTime);

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
    
    display.setFullWindow();
    display.firstPage();

    // The GxEPD2 init sequence doesn't work perfectly for all Waveshare panel revisions,
    // resulting in a washed-out display. We override it here by sending the known-good
    // init sequence from Waveshare's own example code via raw SPI. This is done
    // *after* firstPage() because firstPage() performs its own hardware reset and init.
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    waveshare_init_override();
    SPI.endTransaction();

    do {
        display.fillScreen(GxEPD_WHITE);
        
        drawWeatherPanel(weather, dateTime);
        drawBonsaiPanel();
        
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
    // Release the RTC hold on pins that were isolated during deep sleep.
    // Without this, the ESP32 cannot communicate with the display upon waking up!
    waveshare_board_wakeup_pins();

    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n--- Paper Bonsai Booting ---");
    logWakeupReason();

    setupWeather();
    setupTime();

    // The Waveshare ESP32 Driver board uses alternate SPI pins.
    // We must remap them before initializing the display.
    // SCK=13, MISO=12, MOSI=14, CS=15
    SPI.end(); 
    SPI.begin(13, 12, 14, 15);

    display.init(115200, true, 2, false); // The "true, 2, false" fixes known Waveshare reset issues

    display.setRotation(1); // Landscape
    display.setFont(&FreeMonoBold12pt7b);

    // Initialize UI alignment variables now that the display is set up.
    // This ensures the panel is correctly placed regardless of screen rotation.
    BONSAI_START_X = (display.width() - (BONSAI_WIDTH * FONT_WIDTH)) / 2; // Center horizontally
    BONSAI_START_Y = display.height() - (BONSAI_HEIGHT * FONT_HEIGHT) - 100 + FONT_HEIGHT; // 100px padding from bottom

    // Initial draw
    updateDisplay();

    // Hibernate display to completely power off the panel controller
    display.hibernate();

    // Isolate E-Ink pins during deep sleep to prevent voltage leakage.
    // ESP32 JTAG pins (13, 14, 15) have default pull-ups during deep sleep. This leaks 
    // voltage into the display board, stopping it from properly discharging and resulting 
    // in dim, washed-out colors on subsequent wakeups.
    SPI.end();
    waveshare_board_sleep_pins();

    // Configure deep sleep for 5 minutes (time in microseconds)
    Serial.println("Going to deep sleep for 5 minutes...");
    Serial.flush(); // Ensure serial messages finish transmitting before CPU stops
    esp_sleep_enable_timer_wakeup(5ULL * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
}
