#include <LiquidCrystal_I2C.h>

enum class State {
    Welcome,
    Ready,
    MakingProduct,
    ProductReady,
    Assistance,
    Sleep,
};

const unsigned long MS = 1000;
const unsigned long TIME_MAKING   = 10 * MS;
const unsigned long TIME_TIMEOUT  = 5 * MS;
const unsigned long TIME_IDLE     = 60 * MS;
const unsigned long TIME_CHECK    = 180 * MS;
const unsigned long TEMP_MIN      = 17;
const unsigned long TEMP_MAX      = 24;

// pot          - regolare sugar
// 3 buttons    - Bup, Bdown selects products, Bmake starts making coffee
// lcd          - display
// pir          - checks if there are any users around
// sonar        - checks if coffee was removed
// servo        - simulate machine movements
// temp sensor  - temperature

const int BUTTON_PINS[] = { 8, 9, 10 };
const int SONAR_ECHO_PIN = 3;
const int SONAR_TRIG_PIN = 4;
const int SERVO_PIN = 5;
const int PIR_PIN = 11;

const int POT_PIN = A0;
const int TEMP_PIN = A1;
//const int LCD_WRITE_PIN = 4;
//const int LCD_TARGET_PIN = 5;

const int MAX_PRODUCTS = 10;

LiquidCrystal_I2C lcd(0x27, 16, 2);

struct {
    int state_start;
    State state;

    int last_pir_trig;

    void start(State st) {
        state = st;
        state_start = millis();

        switch (state) {
        case State::Ready:
            last_pir_trig = millis();
            break;
        default:
            break;
        }
    }

    void end() {
    }

    int time() { return millis() - state_start; }
} state_machine;

struct Machine {
    int products[3];
    int cur_product;

    void init() {
        for (int &p : products)
            p = MAX_PRODUCTS;
        cur_product = 0;
    }

    int get_cur() const { return abs(cur_product % 3); }
} machine;

void setup() {
    for (int pin : BUTTON_PINS)
        pinMode(pin, INPUT);
    pinMode(SONAR_ECHO_PIN, INPUT);
    pinMode(SONAR_TRIG_PIN, OUTPUT);
    pinMode(SERVO_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    state_machine.state_start = 0;
    state_machine.state = State::Welcome;
}

void lcd_write(String s)
{
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print(s);
}

void welcome() {
    lcd_write("fox");
    
        Serial.println("time = " + String(state_machine.time()));
    if (state_machine.time() > 1000) {
        machine.init();
        state_machine.start(State::Ready);
    }
}



const int BUTTON_SELECT_INC = 0;
const int BUTTON_SELECT_DEC = 0;
const int BUTTON_CONSUME = 0;

/*
struct {
    bool pressed[3];
    bool just_pressed[3];
    bool just_released[3];
  
    void poll() {
        
    }
} button_handler;
*/

struct {
    bool been_pressed = false;
    int last_press = 0;

    bool pressed(int i) {

        if(been_pressed && digitalRead(BUTTON_PINS[last_press]) == LOW){
          been_pressed = false;
          return false;
        }
        
        if (!been_pressed && digitalRead(BUTTON_PINS[i]) == HIGH) {
            been_pressed = true;
            last_press = i;
            return true;
        }
        return false;
    }
} button_handler;

bool button_checker(int n) {
    static String product_string[] = { "Coffee", "Chocolate", "Tea" };
    if (button_handler.pressed(n)) {
        int inc = n == 0 ? 1 : -1;
        machine.cur_product += inc;
        lcd_write(String("Selected ") + product_string[machine.get_cur()]);
        delay(100);
        return true;
    }
    return false;
}

bool detect_users() { return true; }

void ready() {
    Serial.println("on ready state, cur_product = " + String(machine.cur_product));
    static int button_time = 0;

    if (detect_users())
        state_machine.last_pir_trig = millis();
    if (millis() - state_machine.last_pir_trig > TIME_IDLE)
        state_machine.start(State::Sleep);

    if (button_time <= 0)
        lcd_write("ready");

    button_time = button_checker(0) || button_checker(1) ? 5000 : (button_time > 0 ? button_time - 10 : 0);

    if (button_handler.pressed(2)) {
        state_machine.start(State::MakingProduct);
    }
}

void making_product() {
  
  Serial.println("consumer");
}

void machine_sleep() {
  Serial.println("sleeping");
}

void loop() {
    switch (state_machine.state) {
    case State::Welcome:          welcome(); break;
    case State::Ready:            ready(); break;
    case State::MakingProduct:    making_product(); break;
    case State::ProductReady: break;
    case State::Assistance: break;
    case State::Sleep:            machine_sleep(); break;
    }
    delay(10);
}

int main(void)
{
    arduino_sdl::start("Macchina del Caffè", 800, 600);
    arduino_sdl::connect_button(BUTTON_PINS[0], 100, 100);
    arduino_sdl::connect_button(BUTTON_PINS[1], 100, 200);
    arduino_sdl::connect_button(BUTTON_PINS[2], 150, 150);
    arduino_sdl::connect_lcd(0x27, A4, A5, 16, 2, 200, 200);
    arduino_sdl::loop();
    arduino_sdl::quit();
    return 0;
}
