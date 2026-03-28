#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#include "weather.h"
#include "bonsai.h"

// Define display class for 7.5" V2 3-Color GxEPD2_750c_Z08
// Waveshare ESP32 Driver Board pins: CS=15, DC=27, RST=26, BUSY=25
GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(GxEPD2_750c_Z08(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

// UI alignment variables. These are initialized in setup() after the display
// is rotated, so we can use the correct width and height.
int FONT_WIDTH = 14; 
int FONT_HEIGHT = 24;
int BONSAI_START_X;
int BONSAI_START_Y;

void drawWeatherPanel() {
    WeatherData data = getWeatherData();

    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 60);
    display.print("STATION STATUS");

    display.setCursor(20, 100);
    display.print("Temp: ");
    display.print(data.temperature, 1);
    display.print(" C");

    display.setCursor(20, 140);
    display.print("Hum : ");
    display.print(data.humidity, 1);
    display.print(" %");

    display.setTextColor(GxEPD_RED);
    display.setCursor(20, 180);
    display.print(data.statusMessage);
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
    
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        //drawWeatherPanel();
        drawBonsaiPanel();
        
    } while (display.nextPage());
    
    display.hibernate();
    Serial.println("Refresh complete.");
}

void setup() {
    Serial.begin(115200);
    delay(100);

    setupWeather();

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
    BONSAI_START_X = display.width() - (BONSAI_WIDTH * FONT_WIDTH) - 20; // 20px padding
    BONSAI_START_Y = (display.height() - (BONSAI_HEIGHT * FONT_HEIGHT)) / 2 + FONT_HEIGHT;

    // Initial draw
    updateDisplay();
}

void loop() {
    // Refresh the screen every 5 minutes
    delay(5L * 60L * 1000L); // 5 minutes
    updateDisplay();
}
