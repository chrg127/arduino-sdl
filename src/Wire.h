#pragma once

#include <cstdint>

struct _wire {
    uint8_t cur_addr;

    void begin()                         { cur_addr = 0; }
    void beginTransmission(uint8_t addr) { cur_addr = addr; }
    void endTransmission()               { cur_addr = 0; }
    void write(uint8_t data);
};

extern _wire Wire;
