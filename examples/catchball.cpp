#include <stdint.h>
#include "arduino_sdl.h"

const int LS = 9;
const int leds[] = { 3, 4, 5, 6 };
const int buttons[] = { 2, 7, 8, 10 };

void setup()
{
    Serial.begin(9600);
    pinMode(LS, OUTPUT);
    for (int i = 0; i < 4; i++) {
        pinMode(leds[i], OUTPUT);
    }
    for (int i = 0; i < 4; i++) {
        pinMode(buttons[i], INPUT);
    }
}

void turn_leds_off()
{
    analogWrite(LS, 0);
    for (int i = 0; i < 4; i++) {
        digitalWrite(leds[i], LOW);
    }
}

int blink_leds(int t1)
{
    int tt = t1;
    int curr_pos = -1;
    int dir = 1;
    while (tt > 0) {

        if (curr_pos != -1) {
            digitalWrite(leds[curr_pos], LOW);
        }

        curr_pos += dir;
        if (curr_pos == 3) {
            dir = -1;
        } else if (curr_pos == 0) {
            dir = 1;
        }

        digitalWrite(leds[curr_pos], HIGH);

        tt -= 100;
        delay(100);
    }
    return curr_pos;
}

void initial_state()
{
    while (true) {
        Serial.println("press t1 to begin");
        int seconds = 10000;
        int brightness = 0;
        int fadeAmount = 5;
        while (seconds > 0) {
            analogWrite(LS, brightness);
            brightness = brightness + fadeAmount;
            if (brightness <= 0 || brightness >= 255) {
                fadeAmount = -fadeAmount;
            }
            seconds -= 10;
            delay(10);

            if (digitalRead(buttons[0]) == HIGH) {
                return;
            }
        }

        turn_leds_off();
        Serial.println("zzz...");
        while (true) {
            if (digitalRead(buttons[0]) == HIGH) {
                return;
            }
            if (digitalRead(buttons[1]) == HIGH || digitalRead(buttons[2]) == HIGH || digitalRead(buttons[3]) == HIGH) {
                break;
            }
            delay(50);
        }
    }
}

bool game(int pos, int t2)
{
    int tt = t2;
    while (tt > 0) {

        for (int i = 0; i < 4; i++) {
            if (digitalRead(buttons[i]) == HIGH) {
                return i == pos ? true : false;
            }
        }

        tt -= 50;
        delay(50);
    }
    return false;
}

int difficulties[] = { 10, 12, 15, 18, 21, 24, 27, 30 };
void loop()
{
    initial_state();

    uint32_t t1 = random(4000, 4501);
    uint32_t t2 = 2000; // da settare con potenziometro
    int score = 0;
    int difficulty = analogRead(A0) / 128;
    Serial.println("starting game with diffuculty set as: " + String(difficulty + 1));

    bool state = false;
    do {
        turn_leds_off();
        int curr_pos = blink_leds(t1);
        state = game(curr_pos, t2);
        if (state) {
            score++;
            t1 -= (t1 * difficulties[difficulty] / 100);
            t2 -= (t2 * difficulties[difficulty] / 100);
        }
    } while (state);
    Serial.println("game over");
    Serial.println(score);
    turn_leds_off();

    delay(200);
}

int main()
{
    arduino_sdl::start("Catch the bouncing LED ball", 800, 600);

    int x = 400;
    for (auto pin : leds) {
        arduino_sdl::connect_led(pin, x, 300, arduino_sdl::LedColor::Green);
        x += 64;
    }

    arduino_sdl::connect_led(LS, 100, 100, arduino_sdl::LedColor::Red);

    x = 400;
    for (auto pin : buttons) {
        arduino_sdl::connect_button(pin, x, 400);
        x += 64;
    }

    arduino_sdl::connect_potentiometer(A0, 100, 200);

    arduino_sdl::loop();
    arduino_sdl::quit();
    return 0;
}
