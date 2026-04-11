#include "waveshare_utils.h"

// Helper to send a command and optional data via raw SPI
static void sendCmdData(int8_t cs_pin, int8_t dc_pin, uint8_t cmd, const uint8_t* data, size_t len) {
    digitalWrite(cs_pin, LOW);  // CS LOW
    digitalWrite(dc_pin, LOW);  // DC LOW (Command mode)
    SPI.transfer(cmd);
    if (len > 0 && data != nullptr) {
        digitalWrite(dc_pin, HIGH); // DC HIGH (Data mode)
        for (size_t i = 0; i < len; i++) {
            SPI.transfer(data[i]);
        }
    }
    digitalWrite(cs_pin, HIGH); // CS HIGH
}

// Helper to wait for the BUSY pin to go LOW (idle)
static void waitUntilIdleRaw(int8_t busy_pin) {
    Serial.print("Waiting for BUSY pin to go LOW (idle)...");
    unsigned long start = millis();
    while (digitalRead(busy_pin) == HIGH) { // For V2 panels, BUSY is HIGH when busy
        if (millis() - start > 30000) { // 30s timeout
            Serial.println(" Timeout!");
            return;
        }
        delay(5);
    }
    Serial.println(" OK.");
    delay(5); // Small delay after busy goes high
}

void waveshare_init_override(int8_t cs_pin, int8_t dc_pin, int8_t busy_pin) {
    const uint8_t power_setting[] = {0x07, 0x07, 0x3f, 0x3f};
    sendCmdData(cs_pin, dc_pin, 0x01, power_setting, 4); // POWER SETTING
    
    const uint8_t booster_soft_start[] = {0x17, 0x17, 0x28, 0x17};
    sendCmdData(cs_pin, dc_pin, 0x06, booster_soft_start, 4); // Booster Soft Start
    
    sendCmdData(cs_pin, dc_pin, 0x04, nullptr, 0); // POWER ON
    waitUntilIdleRaw(busy_pin);
    
    const uint8_t pannel_setting[] = {0x1F};
    sendCmdData(cs_pin, dc_pin, 0x00, pannel_setting, 1); // PANNEL SETTING
    
    const uint8_t tres[] = {0x03, 0x20, 0x01, 0xE0};
    sendCmdData(cs_pin, dc_pin, 0x61, tres, 4); // TRES
    
    const uint8_t dual_spi[] = {0x00};
    sendCmdData(cs_pin, dc_pin, 0x15, dual_spi, 1); // Dual SPI
    
    const uint8_t vcom_data_interval[] = {0x10, 0x07};
    sendCmdData(cs_pin, dc_pin, 0x50, vcom_data_interval, 2); // VCOM AND DATA INTERVAL SETTING
    
    const uint8_t tcon_setting[] = {0x22};
    sendCmdData(cs_pin, dc_pin, 0x60, tcon_setting, 1); // TCON SETTING
}

void waveshare_board_wakeup_pins() {
    // Release the GPIO holds that kept the pins stable during deep sleep
    gpio_hold_dis(GPIO_NUM_12);
    gpio_hold_dis(GPIO_NUM_13);
    gpio_hold_dis(GPIO_NUM_14);
    gpio_hold_dis(GPIO_NUM_15);
    gpio_hold_dis(GPIO_NUM_25);
    gpio_hold_dis(GPIO_NUM_26);
    gpio_hold_dis(GPIO_NUM_27);
    gpio_deep_sleep_hold_dis();
}

void waveshare_board_sleep_pins() {
    // We want to physically cut VCC to the E-Ink panel while sleeping.
    // Holding RST LOW for >10ms triggers the Waveshare board's MOSFET to cut power.
    // All other logic pins MUST be pulled LOW to prevent backfeeding voltage into 
    // the display controller. This guarantees zero power draw and lets trapped 
    // DC bias bleed off natively.
    pinMode(15, OUTPUT); digitalWrite(15, LOW);  // CS LOW
    pinMode(27, OUTPUT); digitalWrite(27, LOW);  // DC LOW
    pinMode(26, OUTPUT); digitalWrite(26, LOW);  // RST LOW (CUTS VCC POWER)
    pinMode(13, OUTPUT); digitalWrite(13, LOW);  // SCK LOW
    pinMode(14, OUTPUT); digitalWrite(14, LOW);  // MOSI LOW
    pinMode(12, INPUT);  // MISO
    pinMode(25, INPUT);  // BUSY

    // Engage the holds
    gpio_hold_en(GPIO_NUM_12);
    gpio_hold_en(GPIO_NUM_13);
    gpio_hold_en(GPIO_NUM_14);
    gpio_hold_en(GPIO_NUM_15);
    gpio_hold_en(GPIO_NUM_25);
    gpio_hold_en(GPIO_NUM_26);
    gpio_hold_en(GPIO_NUM_27);
    gpio_deep_sleep_hold_en();
}