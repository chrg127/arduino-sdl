#include "arduino_sdl.h"
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 3); // I2C address 0x27, 16 column and 2 rows

void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.setCursor(1, 0);
  lcd.setCursor(0, 1);
  lcd.print("Arduino");
}

void loop()
{

}

int main(void)
{
    arduino_sdl::start("LCD example", 800, 600);
    arduino_sdl::connect_lcd(0x27, A4, A5, 16, 3, 200, 200);
    arduino_sdl::loop();
    arduino_sdl::quit();
    return 0;
}
