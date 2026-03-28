#ifndef WAVESHARE_UTILS_H
#define WAVESHARE_UTILS_H

#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>

// Sends the full init sequence from the Waveshare example code to fix washed-out display issues.
// Default pins match the Waveshare ESP32 e-Paper Driver Board.
void waveshare_init_override(int8_t cs_pin = 15, int8_t dc_pin = 27, int8_t busy_pin = 25);

// Unlocks the display SPI pins from RTC isolation (call at the very start of setup)
void waveshare_board_wakeup_pins();

// Isolates the display SPI pins to prevent voltage leakage during deep sleep (call right before sleep)
void waveshare_board_sleep_pins();

#endif // WAVESHARE_UTILS_H