#pragma once

#include <cstdint>
#include "arduino_string.h"

struct LiquidCrystal_I2C {
    uint8_t addr, cols, rows;

    LiquidCrystal_I2C(uint8_t addr, uint8_t cols, uint8_t rows);
    void init();
    void backlight();
    void clear();
    void setCursor(int x, int y);
    void print(String s);
};
