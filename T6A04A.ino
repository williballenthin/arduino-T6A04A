#include "T6A04A.h"

// arduino uno r3 pinout
// via: https://www.circuito.io/blog/arduino-uno-pinout/
#define D0 (0)
#define D1 (1)
#define D2 (2)
#define D3 (3)
#define D4 (4)
#define D5 (5)
#define D6 (6)
#define D7 (7)
#define D8 (8)
#define D9 (9)
#define D10 (10)
#define D11 (11)
#define D12 (12)
#define D13 (13) // LED_BUILTIN
#define D14 (14) // A0
#define D15 (15) // A1
#define D16 (16) // A2
#define D17 (17) // A3
#define D18 (18) // A4
#define D19 (19) // A5

// my personal pinout on an Arduino Uno R3
#define LCD_RST D14  // D0 is serial IO pin, D13 is LED_BUILTIN
#define LCD_STB D15
#define LCD_DI D2
#define LCD_CE D3
#define LCD_D7 D4
#define LCD_D6 D5
#define LCD_D5 D6
#define LCD_D4 D7
#define LCD_D3 D8
#define LCD_D2 D9
#define LCD_D1 D10
#define LCD_D0 D11
#define LCD_RW D12

static T6A04A lcd(
    LCD_RST,
    LCD_STB,
    LCD_DI,
    LCD_CE,
    LCD_D7,
    LCD_D6,
    LCD_D5,
    LCD_D4,
    LCD_D3,
    LCD_D2,
    LCD_D1,
    LCD_D0,
    LCD_RW
);

#include "opt.h"

void setup()
{
    Serial.begin(9600);
    run_benchmarks(&lcd);

    lcd.init();
    lcd.clear();

    u8 y = 0;

    for (u8 w = 0; w < 24; w++) {
        lcd.drawFastHLine(0, y, w, true);
        y++;
    }

    for (u8 w = 0; w < 24; w++) {
        lcd.drawFastHLine(2, y, w, true);
        y++;
    }

    for (u8 w = 0; w < 16; w++) {
        lcd.drawFastHLine(8, y, w, true);
        y++;
    }

    for (u8 i = 1; i < 12; i++) {
        lcd.drawPixel(i * 8, 0, true);
    }
}

void loop()
{
    delay(1000);
}