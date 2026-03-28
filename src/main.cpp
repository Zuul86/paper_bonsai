#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#include "weather.h"
#include "bonsai.h"
#include "time.h"

// Define display class for 7.5" V2 3-Color GxEPD2_750c_Z08
// Waveshare ESP32 Driver Board pins: CS=15, DC=27, RST=26, BUSY=25
GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(GxEPD2_750c_Z08(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

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

void updateDisplay() {
    Serial.println("Refreshing E-Ink Display...");
    
    // Generate bonsai once per draw cycle, BEFORE the paging loop
    generateBonsai();
    
    // Fetch weather data once BEFORE the paging loop
    WeatherData weather = getWeatherData();
    String dateTime = getDateTimeString();
    
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        drawWeatherPanel(weather, dateTime);
        drawBonsaiPanel();
        
    } while (display.nextPage());
    
    display.hibernate();
    Serial.println("Refresh complete.");
}

void setup() {
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
}

void loop() {
    // Refresh the screen every 5 minutes
    delay(5L * 60L * 1000L); // 5 minutes
    updateDisplay();
}
