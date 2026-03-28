#include "weather.h"

void setupWeather() {
    // Dummy init. Ready later for Wire.begin() and sensor.begin()
    Serial.println("Sensors Initialized (Dummy)");
}

WeatherData getWeatherData() {
    WeatherData data;
    // Generate some random realistic dummy values
    data.temperature = random(200, 250) / 10.0; // 20.0 to 25.0 C
    data.humidity = random(400, 600) / 10.0; // 40.0% to 60.0%
    data.statusMessage = "All Sensors OK";
    return data;
}
