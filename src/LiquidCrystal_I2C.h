#pragma once

#include "arduino_string.h"

struct LiquidCrystal_I2C {
    int a, b, c;

    LiquidCrystal_I2C(int a, int b, int c);
    void begin();
    void clear();
    void setCursor(int x, int y);
    void print(String s);
};
