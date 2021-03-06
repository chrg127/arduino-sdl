#pragma once

#include <cstdint>
#include "arduino_string.h"

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

#define CHANGE 1
#define FALLING 2
#define RISING 3

#ifdef abs
#undef abs
#endif

//#define min(a,b) ((a)<(b)?(a):(b))
//#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b) (1UL << (b))

using word = unsigned int;
using boolean = bool;
using byte = uint8_t;

const uint8_t A0 = 14;
const uint8_t A1 = 15;
const uint8_t A2 = 16;
const uint8_t A3 = 17;
const uint8_t A4 = 18;
const uint8_t A5 = 19;

struct HardwareSerial {
    void begin(int baud);
    void println(const String &msg);
    void println(const char *msg);
    void println(int n);
};

extern HardwareSerial Serial;

void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t val);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, uint8_t val);
//void analogReference(uint8_t mode);

unsigned long millis();
void delay(unsigned long ms);

void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode);
void detachInterrupt(uint8_t interruptNum);

void setup();
void loop();

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

long random(long n);
long random(long a, long b);
void randomSeed(unsigned long seed);
long map(long x, long in_min, long in_max, long out_min, long out_max);



namespace arduino_sdl {

void start(const char *title, int width, int height);
void loop();
void quit();

enum class PinType {
    Analog, Digital
};

void connect_led(int pin, int x, int y, unsigned color_min, unsigned color_max);
void connect_button(int pin, int x, int y);
void connect_potentiometer(int pin, int x, int y);
void connect_lcd(uint8_t addr, uint8_t sda, uint8_t scl, int c, int r, int x, int y);

} // namespace arduino_sdl
