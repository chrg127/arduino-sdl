#pragma once

#include <cstddef>
#include <cstring>
#include "arduino_sdl.h"

struct Print {
    virtual size_t write(uint8_t) = 0;
    size_t write(const uint8_t *buffer, size_t size);
    size_t print(const char *s) { return write((const uint8_t *)s, strlen(s)); }
    size_t print(String s)      { return write((const uint8_t *)s.c_str(), s.length()); }
};
