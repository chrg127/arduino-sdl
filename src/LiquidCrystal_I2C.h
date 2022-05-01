#pragma once

#include <cstdint>
#include <Print.h>

#define LCD_SETDDRAMADDR 0x80

class LiquidCrystal_I2C : public Print {
    uint8_t addr, cols, rows;

    void command(uint8_t cmd, uint8_t data);

public:
    LiquidCrystal_I2C(uint8_t addr, uint8_t cols, uint8_t rows);
    void init();
    void backlight();
    void noBacklight();
    void clear();
    void setCursor(uint8_t x, uint8_t y);
    size_t write(uint8_t);
};
