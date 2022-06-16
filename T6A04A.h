#ifndef T6A04A_H
#define T6A04A_H

#include <Arduino.h>

typedef unsigned char u8;
typedef u8 pin;

// LCD Pinout (Toshiba T6A04A - 17 pin interface)
//
//    1  VCC [Fat wire 1]       +5
//    2  GND [Fat wire 2]       GND
//    3  RST                    D14
//    4  NC
//    5  NC
//    6  STB                    D15
//    7  DI                     D2
//    8  CE                     D3
//    9  D7                     D4
//    10 D6                     D5
//    11 D5                     D6
//    12 D4                     D7
//    13 D3                     D8
//    14 D2                     D9
//    15 D1                     D10
//    16 D0                     D11
//    17 RW                     D12

const u8 ROW_COUNT = 64;
const u8 COLUMN_COUNT = 8 * 12;

const u8 STANDBY_ENABLE = LOW;
const u8 STANDBY_DISABLE = HIGH;
const u8 RW_WRITE = LOW;
const u8 RW_READ = HIGH;

typedef enum WriteMode {
    INSTRUCTION = 1,
    DATA = 2,
} WriteMode;

typedef enum Direction {
    COLUMN = 1,
    ROW = 2,
} Direction;

class T6A04A
{
private:
    pin rst; // pin 3
    pin stb; // pin 6
    pin di;  // pin 7
    pin ce;  // ...
    pin d7;
    pin d6;
    pin d5;
    pin d4;
    pin d3;
    pin d2;
    pin d1;
    pin d0;
    pin rw; // pin 17

    Direction counter_direction;

    void write(WriteMode m, u8 v)
    {
        bool di = 0;
        if (m == WriteMode::INSTRUCTION) {
            di = LOW;
        } else if (m == WriteMode::DATA) {
            di = HIGH;
        } else {
            abort();
        }

        digitalWrite(this->ce, LOW);
        digitalWrite(this->di, di);

        digitalWrite(this->d0, HIGH && (v & B00000001));
        digitalWrite(this->d1, HIGH && (v & B00000010));
        digitalWrite(this->d2, HIGH && (v & B00000100));
        digitalWrite(this->d3, HIGH && (v & B00001000));
        digitalWrite(this->d4, HIGH && (v & B00010000));
        digitalWrite(this->d5, HIGH && (v & B00100000));
        digitalWrite(this->d6, HIGH && (v & B01000000));
        digitalWrite(this->d7, HIGH && (v & B10000000));

        digitalWrite(this->ce, HIGH);

        // I don't know how long the pulse must be,
        // in practice on an Uno R3, no delay is required.
        // but we have a trivial delay here for documentation.
        delayMicroseconds(1);

        digitalWrite(this->ce, LOW);
    }

    void write_instruction(u8 v)
    {
        write(WriteMode::INSTRUCTION, v);
    }

    void write_data(u8 v)
    {
        write(WriteMode::DATA, v);
    }

    // I'm not sure what the alternative to 8-bits is.
    // perhaps there's a 4-bit mode like described here:
    //
    //   https://embedjournal.com/interface-lcd-in-4-bit-mode/
    //
    // but this driver relies only on the 8-bit interface.
    void set_eight_bits()
    {
        this->write_instruction(0b00000001);
    }

public:
    T6A04A(
        pin rst,
        pin stb,
        pin di,
        pin ce,
        pin d7,
        pin d6,
        pin d5,
        pin d4,
        pin d3,
        pin d2,
        pin d1,
        pin d0,
        pin rw)
        : rst(rst),
          stb(stb),
          di(di),
          ce(ce),
          d7(d7),
          d6(d6),
          d5(d5),
          d4(d4),
          d3(d3),
          d2(d2),
          d1(d1),
          d0(d0),
          rw(rw),
          counter_direction(Direction::ROW)
    {
        pinMode(this->ce, OUTPUT);
        pinMode(this->di, OUTPUT);
        pinMode(this->rw, OUTPUT);
        pinMode(this->rst, OUTPUT);
        pinMode(this->d0, OUTPUT);
        pinMode(this->d1, OUTPUT);
        pinMode(this->d2, OUTPUT);
        pinMode(this->d3, OUTPUT);
        pinMode(this->d4, OUTPUT);
        pinMode(this->d5, OUTPUT);
        pinMode(this->d6, OUTPUT);
        pinMode(this->d7, OUTPUT);

        // the LCD is reset when RST is pulsed low.
        digitalWrite(this->rst, HIGH);

        digitalWrite(this->stb, STANDBY_DISABLE);

        // unknown how to read from the LCD, so all operations are write in this driver.
        digitalWrite(this->rw, RW_WRITE);
    }

    void init() {
        this->reset();

        // this driver only uses the 8-bit interface.
        this->set_eight_bits();

        this->enable_display();
        this->set_contrast(48);
        this->set_counter_direction(Direction::ROW);
        this->set_x(0);
        this->set_y(0);
    }

    void reset() {
        digitalWrite(this->rst, LOW);
        delay(1);
        digitalWrite(this->rst, HIGH);
    }

    void enable_standby() {
        digitalWrite(this->stb, STANDBY_ENABLE);
    }

    void disable_standby() {
        digitalWrite(this->stb, STANDBY_DISABLE);
    }

    void set_contrast(u8 contrast)
    {
        this->write_instruction(0b11000000 | (contrast & 0b00111111));
    }

    void enable_display()
    {
        this->write_instruction(0b00000011);
    }

    void disable_display()
    {
        this->write_instruction(0b00000010);
    }

    void set_counter_direction(Direction d)
    {
        this->counter_direction = d;
        if (d == Direction::COLUMN) {
            // TODO: bit 0x1 is used for something
            // called "up" in the TILCD code.
            // but I don't understand it enough to document or expose it yet.
            this->write_instruction(0b00000101);
        } else if (d == Direction::ROW) {
            this->write_instruction(0b00000111);
        } else {
            abort();
        }
    }

    // set the x coordinate for subsequent call to `write_byte`.
    // note that the unit here is in 8-bit bytes, not pixels.
    void set_x(u8 x)
    {
        this->write_instruction(0b00100000 | (x & 0b00011111));
    }

    // set the y coordinate for subsequent calls to `write_byte`.
    // unit: pixels.
    void set_y(u8 y)
    {
        this->write_instruction(0b10000000 | (y & 0b00111111));
    }

    // TODO: i dont understand what z is.
    // seems to have to do with shifting the screen.
    void set_z(u8 z)
    {
        this->write_instruction(0b01000000 | (z & 0b00111111));
    }

    // write 8-bits to the LCD, left-to-right.
    // MSB is the leftmost pixel, LSB is the rightmost pixel.
    //
    // the LCD driver will then increment the counter;
    // if in COLUMN counter mode, the y coordinate will be incremented by one pixel.
    // if in ROW counter mode, the x coordinate will be incremented by one byte (8 pixels).
    void write_byte(u8 v)
    {
        this->write_data(v);
    }

    void clear()
    {
        Direction d = this->counter_direction;

        if (d == Direction::COLUMN) {
            this->set_counter_direction(Direction::ROW);
        }

        for (int y = 0; y < ROW_COUNT; y++) {
            this->set_y(y);
            this->set_x(0);
            for (int x = 0; x < (COLUMN_COUNT / 8); x++) {
                this->write_byte(0);
            }
        }

        if (d == Direction::COLUMN) {
            this->set_counter_direction(Direction::COLUMN);
        }
    }
};

class PixelCanvas {
private:
    u8 buffer[ROW_COUNT][COLUMN_COUNT/8];

public:
    T6A04A inner;

    PixelCanvas(T6A04A inner)
        : inner(inner)
    {
        for (u8 row = 0; row < ROW_COUNT; row++) {
            for (u8 column = 0; column < COLUMN_COUNT; column++) {
                this->buffer[row][column] = 0;
            }
        }
    }

    void write_pixel(u8 x, u8 y, bool on)
    {
        if (x >= COLUMN_COUNT || y >= ROW_COUNT) {
            return;
        }

        u8 (&row)[COLUMN_COUNT/8] = (this->buffer[y]);
        u8 column = x / 8;
        u8 bit = x % 8;

        u8 existing = row[column];
        u8 next = existing;
        if (on) {
            next |= (1 << bit);
        } else {
            next &= ~(1 << bit);
        }

        if (next != existing) {
            inner.set_y(y);
            inner.set_x(column);
            inner.write_byte(next);
        }
    }

    void clear()
    {
        for (u8 row = 0; row < ROW_COUNT; row++) {
            for (u8 column = 0; column < COLUMN_COUNT/8; column++) {
                if (0 != this->buffer[row][column]) {
                    this->buffer[row][column] = 0;
                    this->inner.set_y(row);
                    this->inner.set_x(column);
                    this->inner.write_byte(0x00);
                }
            }
        }
    }
};

#endif // T6A04A_H