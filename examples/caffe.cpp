#include <LiquidCrystal_I2C.h>

enum class State {
    Welcome,
    Ready,
    MakingProduct,
    ProductReady,
    Assistance,
    Sleep,
};

const int MS = 1000;
const int TIME_MAKING   = 10 * MS;
const int TIME_TIMEOUT  = 5 * MS;
const int TIME_IDLE     = 60 * MS;
const int TIME_CHECK    = 180 * MS;
const int TEMP_MIN      = 17;
const int TEMP_MAX      = 24;

// pot          - regolare sugar
// 3 buttons    - Bup, Bdown selects products, Bmake starts making coffee
// lcd          - display
// pir          - checks if there are any users around
// sonar        - checks if coffee was removed
// servo        - simulate machine movements
// temp sensor  - temperature


const int BUTTON_PINS[] = { 0, 1, 2 };
const int SONAR_ECHO_PIN = 3;
const int SONAR_TRIG_PIN = 4;
const int SERVO_PIN = 5;
const int PIR_PIN = 9;

const int POT_PIN = A0;
const int TEMP_PIN = A1;
//const int LCD_WRITE_PIN = 4;
//const int LCD_TARGET_PIN = 5;

const int MAX_PRODUCTS = 10;

LiquidCrystal_I2C lcd(0x27, 16, 2);

struct {
    int start;
    State state;

    int last_pir_trig;

    void start(State st) {
        state = st;
        start = millis();

        switch (state) {
        case State::Ready:
            last_pir_trig = millis();
            break;
        }
    }

    void end() {
    }

    void time() { return millis() - start; }
} state_machine;

struct Machine {
    int products[3];
    int cur_product;

    void init() {
        for (int &p : products)
            p = MAX_PRODUCTS;
        cur_product = 0;
    }
} machine;

void setup() {
    for (int pin : BUTTON_PINS)
        pinMode(pin, INPUT);
    pinMode(SONAR_ECHO_PIN, INPUT);
    pinMode(SONAR_TRIG_PIN, OUTPUT);
    pinMode(SERVO_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    Serial.begin(9600);
    lcd.begin();
    machine.state_start = 0;
    state_machine.state = State::Welcome;
}

void lcd_write(String s)
{
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print(s);
}

bool button_pressed(int idx) { return digitalRead(BUTTON_PINS[idx]) == HIGH; }

void welcome() {
    lcd_write("fox");
    if (state_machine.time() > 1000) {
        machine.init();
        state_machine.start(State::Ready);
    }
}

bool button_checker(int n) {
    if (button_pressed(n)) {
        int inc = n == 0 ? 1 : -1;
        machine.cur_product = (machine.cur_product + inc) % 3;
        lcd_write(String("Selected") + product_string[machine.cur_product]);
        delay(36);
        returrn true;
    }
    returrn false;
}

void ready() {
    static String product_string[] = { "Coffee", "Chocolate", "Tea" };
    static int button_time = 0;

    if (button_time <= 0)
        lcd_write("ready");

    // dai rizzi prova a capire questo
    button_time = button_checker(0) || button_checker(1) ? 5000 : (button_time > 0 ? button_time - 10 : 0);



    if (detect_users())
        state_machine.last_pir_trig = millis();
    if (millis() - state_machine.last_pir_trig > TIME_IDLE)
        state_machine.start(State::Sleep);

    if (button_pressed(2)) {
        state_machine.start(State::MakingProduct);
    }
}

void loop() {
    switch (state_machine.state) {
    case State::Welcome:            welcome(); break;
    case State::Ready: break;
    case State::MakingProduct: break;
    case State::ProductReady: break;
    case State::Assistance: break;
    case State::Sleep: break;
    }
    delay(10);
}
