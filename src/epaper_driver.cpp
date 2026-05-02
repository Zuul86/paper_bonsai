#include "epaper_driver.h"
#include "waveshare_utils.h"
#include <SPI.h>

EPaperDriver::EPaperDriver() : 
    display(GxEPD2_750c_Z08_Custom(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25)) {}

void EPaperDriver::wakeHardware() {
    // Initialize safe logic states BEFORE releasing the RTC holds to prevent phantom power leakage
    pinMode(15, OUTPUT); digitalWrite(15, HIGH); // CS HIGH (Deselected)
    pinMode(27, OUTPUT); digitalWrite(27, HIGH); // DC HIGH (Data mode)
    pinMode(13, OUTPUT); digitalWrite(13, LOW);  // SCK LOW (Idle)
    pinMode(14, OUTPUT); digitalWrite(14, LOW);  // MOSI LOW (Idle)
    pinMode(26, OUTPUT); digitalWrite(26, HIGH); // RST HIGH (Turn on VCC MOSFET)

    waveshare_board_wakeup_pins();
    delay(200); // Give panel capacitors time to stabilize clean VCC
}

void EPaperDriver::initDisplay() {
    SPI.end(); 
    SPI.begin(13, 12, 14, -1);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        display.init(115200, true, 2, false);
        display.clearScreen(); // Forces a full cycle to pristine white on cold boot
    } else {
        display.init(115200, true, 2, false);
    }
    display.setRotation(1); // Landscape
}

void EPaperDriver::sleep() {
    display.hibernate();
    delay(250); // Give internal charge pumps time to bleed off
    SPI.end();
    waveshare_board_sleep_pins();
}

EPaperDisplayType& EPaperDriver::getDisplay() {
    return display;
}