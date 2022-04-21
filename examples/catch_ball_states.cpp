#include <stdint.h>
#include "arduino_sdl.h"

const int RED_LED = 9;
const int LEDS[] = { 3, 4, 5, 6 };
const int BUTTONS[] = { 2, 7, 8, 10 };
const int DIFFICULTIES[] = { 10, 12, 15, 18, 21, 24, 27, 30 };
const int POT = A0;

const int FADE_AMOUNT = 10;

enum { T1, T2, T3, T4 };

enum class State {
    Start, Sleep, Bouncing, Stopped,
};

struct Game {
    State state;
    unsigned long state_time_start;

    /* game-wide variables */
    int difficulty;
    int score;
    int pos;
    int dir;
    unsigned long bouncing_time;
    unsigned long blink_time;

    /* used by start state */
    int light;
    int light_dir;

    /* used by bouncing state */
    unsigned long blink_start;

    /* used by stopped state */
    unsigned long reaction_time;
} game;

bool pressed(int button_no)
{
    return digitalRead(BUTTONS[button_no]) == HIGH;
}

void turn_leds_off()
{
    analogWrite(RED_LED, 0);
    for (int pin : LEDS)
        digitalWrite(pin, LOW);
}

template <typename T>
T percent(T value, T by)
{
    return value * by / 100;
}

void new_game()
{
    game.difficulty = map(analogRead(POT), 0, 1023, 0, 7);
    game.score = 0;
    game.pos = 0;
    game.dir = 1;
    game.bouncing_time = random(4000, 4501);
    game.reaction_time = 2000;
    game.blink_time = 150;
    Serial.println("Difficulty for this game is: " + String(game.difficulty + 1));
    Serial.println("Go!");
}

void update_score()
{
    game.score++;
    Serial.println("New point! Score: " + String(game.score));
    // decrease parameters by a % based on the difficulty
    game.bouncing_time -= percent<unsigned long>(game.bouncing_time, DIFFICULTIES[game.difficulty]);
    game.reaction_time -= percent<unsigned long>(game.reaction_time, DIFFICULTIES[game.difficulty]);
    game.blink_time    -= percent<unsigned long>(game.blink_time,    DIFFICULTIES[game.difficulty]);
}

void end_game()
{
    Serial.println("Game Over. Final Score: " + String(game.score));
    turn_leds_off();
}

void enter_start(bool print_message)
{
    game.state = State::Start;
    game.state_time_start = millis();
    game.light = 0;
    game.light_dir = 1;
    turn_leds_off();
    if (print_message)
        Serial.println("Welcome to the Catch the Bouncing Led Ball game. Press key T1 to start.");
}

void enter_sleep()
{
    game.state = State::Sleep;
    game.state_time_start = millis();
    turn_leds_off();
    Serial.println("entering deep sleep...");
}

void enter_bouncing()
{
    game.state = State::Bouncing;
    game.state_time_start = millis();
    game.blink_start = millis();
    analogWrite(RED_LED, 0);
}

void enter_stopped()
{
    game.state = State::Stopped;
    game.state_time_start = millis();
}

void state_start()
{
    game.light += FADE_AMOUNT * game.light_dir;
    if (game.light <= 0 || game.light >= 255)
        game.light_dir *= -1;
    analogWrite(RED_LED, constrain(game.light, 0, 255));
    auto elapsed = millis() - game.state_time_start;
    if (elapsed / 1000 >= 10) {
        enter_sleep();
    } else if (pressed(T1)) {
        new_game();
        enter_bouncing();
    }
}

void state_sleeping()
{
    if (pressed(T1))
        enter_bouncing();
    if (pressed(T2) || pressed(T3) || pressed(T4))
        enter_start(/* print message = */ false);
}

void state_bouncing()
{
    auto blink_elapsed = millis() - game.blink_start;
    if (blink_elapsed >= game.blink_time) {
        game.blink_start = millis();
        digitalWrite(LEDS[game.pos], LOW);
        game.pos += game.dir;
        game.dir = game.pos == 3 ? -1 : game.pos == 0 ? 1 : game.dir;
        digitalWrite(LEDS[game.pos], HIGH);
    }
    auto elapsed = millis() - game.state_time_start;
    if (elapsed >= game.bouncing_time)
        enter_stopped();
}

void state_stopped()
{
    for (int i = 0; i < 4; i++) {
        if (pressed(T1 + i)) {
            if (i == game.pos) {
                update_score();
                enter_bouncing();
            } else {
                end_game();
                enter_start(/* print message = */ true);
            }
        }
    }
    auto elapsed = millis() - game.state_time_start;
    if (elapsed >= game.reaction_time) {
        end_game();
        enter_start(/* print message = */ true);
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(RED_LED, OUTPUT);
    for (int pin : LEDS)
        pinMode(pin, OUTPUT);
    for (int pin : BUTTONS)
        pinMode(pin, INPUT);
    turn_leds_off();
    enter_start(/* print message = */ true);
}

void loop()
{
    switch (game.state) {
    case State::Start:    state_start(); break;
    case State::Bouncing: state_bouncing(); break;
    case State::Sleep:    state_sleeping(); break;
    case State::Stopped:  state_stopped(); break;
    }
    delay(16);
}

int main()
{
    arduino_sdl::start("Catch the bouncing LED ball", 800, 600);

    int x = 400;
    for (auto pin : LEDS) {
        arduino_sdl::connect_led(pin, x, 300, arduino_sdl::LedColor::Green);
        x += 64;
    }

    arduino_sdl::connect_led(RED_LED, 100, 100, arduino_sdl::LedColor::Red);

    x = 400;
    for (auto pin : BUTTONS) {
        arduino_sdl::connect_button(pin, x, 400);
        x += 64;
    }

    arduino_sdl::connect_potentiometer(A0, 100, 200);

    arduino_sdl::loop();
    arduino_sdl::quit();
    return 0;
}
