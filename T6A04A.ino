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

#define LCD_RST D14
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

static PixelCanvas canvas(
    T6A04A(
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
    )
);

void setup()
{
    Serial.begin(9600);

    canvas.init();
    canvas.clear();

    Serial.println("init done");

    // demonstrate status read
    Status s = canvas.inner.read_status();

    if (s.counter_direction() != Direction::ROW) {
        Serial.println("unexpected counter direction");
    }

    if (s.word_length() != WordLength::WORD_LENGTH_8) {
        Serial.println("unexpected word length");
    }

    if (!s.is_enabled()) {
        Serial.println("unexpected status");
    }

    if (s.is_busy()) {
        Serial.println("unexpected busy");
    }

    Serial.println("checks done");
}

static bool is_on = false;

void loop()
{
    if (is_on) {
        Serial.println("on");

        for (u8 i = 0; i < 64; i++) {
            canvas.write_pixel(i, i, true);
        }
    } else {
        Serial.println("off");
        canvas.clear();
    }

    is_on = !is_on;

    delay(1000);
}
