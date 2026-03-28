#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <driver/rtc_io.h>

#include "weather.h"
#include "bonsai.h"
#include "time_sync.h"

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

    // Date and Time in center with smaller font
    display.setFont(NULL); // Use default 6x8 font
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTime, 0, 0, &x1, &y1, &w, &h);
    int dateTimeX = (display.width() - w) / 2;
    // The 12pt font's baseline is at y=30. The default font's top is at cursor_y.
    // The default font is 8px high. To align its bottom with the 12pt baseline:
    int dateTimeY = 30 - 8;
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


// Helper to send a command and optional data via raw SPI
void sendCmdData(uint8_t cmd, const uint8_t* data, size_t len) {
    digitalWrite(15, LOW);  // CS LOW
    digitalWrite(27, LOW);  // DC LOW (Command mode)
    SPI.transfer(cmd);
    if (len > 0) {
        digitalWrite(27, HIGH); // DC HIGH (Data mode)
        for (size_t i = 0; i < len; i++) {
            SPI.transfer(data[i]);
        }
    }
    digitalWrite(15, HIGH); // CS HIGH
}

// Helper to wait for the BUSY pin to go LOW (idle)
void waitUntilIdleRaw() {
    Serial.print("Waiting for BUSY pin to go LOW (idle)...");
    unsigned long start = millis();
    while (digitalRead(25) == HIGH) { // For V2 panels, BUSY is HIGH when busy
        if (millis() - start > 30000) { // 30s timeout
            Serial.println(" Timeout!");
            return;
        }
        delay(5);
    }
    Serial.println(" OK.");
    delay(5); // Small delay after busy goes low
}

// Sends the full init sequence from the Waveshare example code to fix washed-out display issues.
void waveshare_init_override() {
    sendCmdData(0x01, (const uint8_t[]){0x07, 0x07, 0x3f, 0x3f}, 4); // POWER SETTING
    sendCmdData(0x06, (const uint8_t[]){0x17, 0x17, 0x28, 0x17}, 4); // Booster Soft Start
    sendCmdData(0x04, NULL, 0);                                      // POWER ON
    waitUntilIdleRaw();
    sendCmdData(0x00, (const uint8_t[]){0x1F}, 1);                    // PANNEL SETTING
    sendCmdData(0x61, (const uint8_t[]){0x03, 0x20, 0x01, 0xE0}, 4); // TRES
    sendCmdData(0x15, (const uint8_t[]){0x00}, 1);                    // Dual SPI
    sendCmdData(0x50, (const uint8_t[]){0x10, 0x07}, 2);              // VCOM AND DATA INTERVAL SETTING (The "gray screen" fix)
    sendCmdData(0x60, (const uint8_t[]){0x22}, 1);                    // TCON SETTING
}

void updateDisplay() {
    Serial.println("Refreshing E-Ink Display...");
    
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
    
    Serial.println("Refresh complete.");
}

void setup() {
    // Release the RTC hold on pins that were isolated during deep sleep.
    // Without this, the ESP32 cannot communicate with the display upon waking up!
    rtc_gpio_hold_dis(GPIO_NUM_12);
    rtc_gpio_hold_dis(GPIO_NUM_13);
    rtc_gpio_hold_dis(GPIO_NUM_14);
    rtc_gpio_hold_dis(GPIO_NUM_15);
    rtc_gpio_hold_dis(GPIO_NUM_25);
    rtc_gpio_hold_dis(GPIO_NUM_26);
    rtc_gpio_hold_dis(GPIO_NUM_27);

    Serial.begin(115200);
    delay(100);

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
    rtc_gpio_isolate(GPIO_NUM_12); // MISO
    rtc_gpio_isolate(GPIO_NUM_13); // SCK
    rtc_gpio_isolate(GPIO_NUM_14); // MOSI
    rtc_gpio_isolate(GPIO_NUM_15); // CS
    rtc_gpio_isolate(GPIO_NUM_25); // BUSY
    rtc_gpio_isolate(GPIO_NUM_26); // RST
    rtc_gpio_isolate(GPIO_NUM_27); // DC

    // Configure deep sleep for 5 minutes (time in microseconds)
    Serial.println("Going to deep sleep for 5 minutes...");
    Serial.flush(); // Ensure serial messages finish transmitting before CPU stops
    esp_sleep_enable_timer_wakeup(5ULL * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
}
