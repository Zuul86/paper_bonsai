#include "weather.h"
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22 // Change to DHT11 if you are using a DHT11

DHT dht(DHTPIN, DHTTYPE);

void setupWeather() {
    dht.begin();
    Serial.println("DHT Sensor Initialized on pin 4");
}

WeatherData getWeatherData() {
    WeatherData data;
    data.temperature = 0.0f;
    data.humidity = 0.0f;
    data.statusMessage = "Initializing...";
    
    Serial.println("Reading DHT sensor...");
    float h = dht.readHumidity();
    float t = dht.readTemperature(true); // Read temperature in Fahrenheit

    // Check if any reads failed and set the status accordingly
    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        data.statusMessage = "Sensor Error";
    } else {
        data.temperature = t;
        data.humidity = h;
        data.statusMessage = "All Sensors OK";
        Serial.printf("DHT read successful: %.1f F, %.1f %%\n", t, h);
    }

    return data;
}
