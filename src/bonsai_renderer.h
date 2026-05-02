#ifndef BONSAI_RENDERER_H
#define BONSAI_RENDERER_H

#include <Arduino.h>
#include "epaper_driver.h"
#include "weather.h"
#include "bonsai.h"

class BonsaiRenderer {
public:
    BonsaiRenderer(EPaperDisplayType& display);
    void initDimensions();
    void drawWeatherPanel(const WeatherData& data, const String& dateTime, float voltage);
    void drawBonsaiPanel();

private:
    EPaperDisplayType& display;
    int FONT_WIDTH = 14; 
    int FONT_HEIGHT = 24;
    int BONSAI_START_X = 0;
    int BONSAI_START_Y = 0;
};

#endif // BONSAI_RENDERER_H