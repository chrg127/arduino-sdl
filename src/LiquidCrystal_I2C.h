#pragma once

#include <cstdint>
#include "arduino_string.h"

#define LCD_SETDDRAMADDR 0x80

struct LiquidCrystal_I2C {
    uint8_t addr, cols, rows;

    LiquidCrystal_I2C(uint8_t addr, uint8_t cols, uint8_t rows);
    void init();
    void backlight();
    void clear();
    void setCursor(uint8_t x, uint8_t y);
    void print(String s);
};
