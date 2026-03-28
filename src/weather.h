#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

struct WeatherData {
    float temperature;
    float humidity;
    String statusMessage;
};

// Initializes the dummy sensors (can be replaced with actual e.g. BME280 init)
void setupWeather();

// Returns the current struct with populated data
WeatherData getWeatherData();

#endif
