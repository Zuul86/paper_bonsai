#include "bonsai_renderer.h"
#include <Fonts/FreeMonoBold12pt7b.h>

BonsaiRenderer::BonsaiRenderer(EPaperDisplayType& display) 
    : display(display) {}

void BonsaiRenderer::initDimensions() {
    display.setFont(&FreeMonoBold12pt7b);
    BONSAI_START_X = (display.width() - (BONSAI_WIDTH * FONT_WIDTH)) / 2; // Center horizontally
    BONSAI_START_Y = display.height() - (BONSAI_HEIGHT * FONT_HEIGHT) - 100 + FONT_HEIGHT; // 100px padding from bottom
}

void BonsaiRenderer::drawWeatherPanel(const WeatherData& data, const String& dateTime, float voltage) {
    display.setTextColor(GxEPD_BLACK);
    
    // Temperature in upper left
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 30);
    display.print(data.temperature, 1);
    display.print(" F");

    Serial.println(data.statusMessage + " F");
    Serial.printf("Weather Panel Data - Temp: %.1f F, Humidity: %.1f %%, Status: %s\n", data.temperature, data.humidity, data.statusMessage.c_str());

    // Date and Time in bottom right corner (5px padding)
    display.setFont(NULL); // Use default 6x8 font
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTime, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(display.width() - w - 5, display.height() - h - 5);
    display.print(dateTime);

    // Voltage in bottom left corner (5px padding)
    String volStr = String(voltage, 2) + " V";
    display.getTextBounds(volStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(5, display.height() - h - 5);
    display.print(volStr);

    // Humidity in upper right
    display.setFont(&FreeMonoBold12pt7b); 
    int humX = display.width() - (7 * FONT_WIDTH) - 20;
    display.setCursor(humX, 30);
    display.print(data.humidity, 1);
    display.print(" %");
}

void BonsaiRenderer::drawBonsaiPanel() {
    for (int y = 0; y < BONSAI_HEIGHT; y++) {
        for (int x = 0; x < BONSAI_WIDTH; x++) {
            BonsaiCell cell = getBonsaiCell(x, y);
            
            if (cell.ch != ' ') {
                if (cell.type == TYPE_LEAF) {
                    bool isRed = (x * 13 + y * 7) % 3 == 0;
                    display.setTextColor(isRed ? GxEPD_RED : GxEPD_BLACK);
                } else {
                    display.setTextColor(GxEPD_BLACK);
                }
                display.setCursor(BONSAI_START_X + (x * FONT_WIDTH), BONSAI_START_Y + (y * FONT_HEIGHT));
                display.print(cell.ch);
            }
        }
    }
}