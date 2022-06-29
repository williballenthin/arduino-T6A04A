/*
 * TODO:
 *  - refactor counter handling
 *  - drawFastVLine
 *  - drawRect
 */

#ifndef T6A04A_H
#define T6A04A_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

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
const u8 Y_COUNT = ROW_COUNT;
const u8 X_COUNT = 96;

const u8 STANDBY_ENABLE = LOW;
const u8 STANDBY_DISABLE = HIGH;
const u8 RW_WRITE = LOW;
const u8 RW_READ = HIGH;

typedef enum WriteMode {
    WRITE_INSTRUCTION = 1,
    WRITE_DATA = 2,
} WriteMode;

typedef enum ReadMode {
    READ_STATUS = 1,
    READ_DATA = 2,
} ReadMode;

typedef enum Direction {
    COLUMN = 1,
    ROW = 2,
} Direction;

typedef enum WordLength {
    WORD_LENGTH_8 = 1,
    WORD_LENGTH_6 = 2,
} WordLength;

// used to hold OUTPUT, INPUT
typedef u8 IOMode;

class Status {
private:
    u8 inner;
public:
    Status(u8 v) : inner(v) {}

    bool is_busy() const
    {
        return (this->inner & 0b10000000) != 0;
    }

    WordLength word_length() const
    {
        if ((this->inner & 0b01000000) != 0) {
            return WordLength::WORD_LENGTH_8;
        } else {
            return WordLength::WORD_LENGTH_6;
        };
    }

    bool is_enabled() const
    {
        return (this->inner & 0b00100000) != 0;
    }

    Direction counter_direction() const
    {
        if ((this->inner & 0b00000010) != 0) {
            return Direction::ROW;
        } else {
            return Direction::COLUMN;
        };
    }

    // up/down is the LSB
    // 1: up, 0: down
};



class T6A04A : public Adafruit_GFX
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
    WordLength word_length;
    IOMode io_mode;

    void bus_write(WriteMode m, u8 v)
    {
        bool di = 0;
        if (m == WriteMode::WRITE_INSTRUCTION) {
            di = LOW;
        } else if (m == WriteMode::WRITE_DATA) {
            di = HIGH;
        } else {
            abort();
        }

        digitalWrite(this->ce, LOW);
        digitalWrite(this->di, di);
        digitalWrite(this->rw, LOW);
        this->set_bus_mode(OUTPUT);

        digitalWrite(this->d0, HIGH && (v & B00000001));
        digitalWrite(this->d1, HIGH && (v & B00000010));
        digitalWrite(this->d2, HIGH && (v & B00000100));
        digitalWrite(this->d3, HIGH && (v & B00001000));
        digitalWrite(this->d4, HIGH && (v & B00010000));
        digitalWrite(this->d5, HIGH && (v & B00100000));
        digitalWrite(this->d6, HIGH && (v & B01000000));
        digitalWrite(this->d7, HIGH && (v & B10000000));

        digitalWrite(this->ce, HIGH);

        // "As mentioned, a 10 microsecond delay is required after sending the command"
        // via: https://wikiti.brandonw.net/index.php?title=83Plus:Ports:10
        delayMicroseconds(10);

        digitalWrite(this->ce, LOW);
    }

    void write_instruction(u8 v)
    {
        this->bus_write(WriteMode::WRITE_INSTRUCTION, v);
    }

    void write_data(u8 v)
    {
        this->bus_write(WriteMode::WRITE_DATA, v);
    }

    void set_bus_mode(IOMode m)
    {
        if (m != this->io_mode) {
            pinMode(this->d0, m);
            pinMode(this->d1, m);
            pinMode(this->d2, m);
            pinMode(this->d3, m);
            pinMode(this->d4, m);
            pinMode(this->d5, m);
            pinMode(this->d6, m);
            pinMode(this->d7, m);

            this->io_mode = m;
        }
    }

    u8 bus_read(ReadMode m)
    {
        digitalWrite(this->ce, LOW);
        if (ReadMode::READ_STATUS == m) {
            digitalWrite(this->di, LOW);
        } else if (ReadMode::READ_DATA == m) {
            digitalWrite(this->di, HIGH);
        } else {
            abort();
        }
        digitalWrite(this->rw, HIGH);
        this->set_bus_mode(INPUT);

        digitalWrite(this->ce, HIGH);

        // "As mentioned, a 10 microsecond delay is required after sending the command"
        // via: https://wikiti.brandonw.net/index.php?title=83Plus:Ports:10
        delayMicroseconds(10);

        const u8 d0 = digitalRead(this->d0);
        const u8 d1 = digitalRead(this->d1);
        const u8 d2 = digitalRead(this->d2);
        const u8 d3 = digitalRead(this->d3);
        const u8 d4 = digitalRead(this->d4);
        const u8 d5 = digitalRead(this->d5);
        const u8 d6 = digitalRead(this->d6);
        const u8 d7 = digitalRead(this->d7);

        digitalWrite(this->ce, LOW);

        return (
            (d7 << 7) | 
            (d6 << 6) | 
            (d5 << 5) | 
            (d4 << 4) | 
            (d3 << 3) | 
            (d2 << 2) | 
            (d1 << 1) | 
            d0
        );
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
          counter_direction(Direction::ROW),
          word_length(WordLength::WORD_LENGTH_8),
          io_mode(OUTPUT),
          Adafruit_GFX(96, 64)
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

        this->set_word_length(WordLength::WORD_LENGTH_8);

        this->enable_display();
        this->set_contrast(48);
        this->set_counter_direction(Direction::ROW);
        this->set_column(0);
        this->set_row(0);
        this->set_z(0);
    }

    // > When /RST = L, the reset function is executed and the following settings are mode. 
    // > (3)     Display..............................OFF     
    // > (4)     Word length..........................8 bits/word 
    // > (5)     Counter mode.........................Y-counter (row-counter)/up mode 
    // > (6)     Y-(page) address.....................Page = 0 (column 0)
    // > (7)     X-address............................XAD  = 0 (row 0)
    // > (8)     Z-address............................ZAD  = 0 
    // > (9)     Op-amp1 (OPA1) ......................min     
    // > (10)    Op-amp2 (OPA2) ......................min   
    void reset() {
        digitalWrite(this->rst, LOW);

        // "As mentioned, a 10 microsecond delay is required after sending the command"
        // via: https://wikiti.brandonw.net/index.php?title=83Plus:Ports:10
        delayMicroseconds(10);

        digitalWrite(this->rst, HIGH);
    }

    // set the word length used when write/reading data to the display.
    // this seems to be useful to update either 8 or 6 bit regions quickly,
    // e.g. when a character is 8 or 6 pixels wide.
    //
    // this doesn't affect the total number of required pins,
    // only the display word size.
    //
    // command: 86E
    //
    // cost: one bus operation
    void set_word_length(WordLength wl)
    {
        this->word_length = wl;
        if (wl == WordLength::WORD_LENGTH_8) {
            this->write_instruction(0b00000001);
        } else if (wl == WordLength::WORD_LENGTH_6) {
            this->write_instruction(0b00000000);
        } else {
            abort();
        }
    }

    // > When /STB = L, the T6A04A is in standby state.
    // > The internal oscillator is stopped, power consumption is 
    // > reduced, and the power supply level for the LCD (VLC1 to VLC5) becomes VDD.
    void enable_standby() {
        digitalWrite(this->stb, STANDBY_ENABLE);
    }

    // > When /STB = L, the T6A04A is in standby state.
    // > The internal oscillator is stopped, power consumption is 
    // > reduced, and the power supply level for the LCD (VLC1 to VLC5) becomes VDD.
    void disable_standby() {
        digitalWrite(this->stb, STANDBY_DISABLE);
    }

    // > This command sets the contrast for the LCD. 
    // > The LCD contrast can be set in 64 steps. 
    //
    // command: SCE
    //
    // cost: one bus operation
    void set_contrast(u8 contrast)
    {
        this->write_instruction(0b11000000 | (contrast & 0b00111111));
    }

    // > This command turns display ON/OFF. It does not affect the data in the display RAM.
    //
    // command: DPE (ON)
    //
    // cost: one bus operation
    void enable_display()
    {
        this->write_instruction(0b00000011);
    }

    // > This command turns display ON/OFF. It does not affect the data in the display RAM.
    //
    // command: DPE (OFF)
    //
    // cost: one bus operation
    void disable_display()
    {
        this->write_instruction(0b00000010);
    }

    // TODO: refactor: x/y up/down
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

    // set the column coordinate for subsequent call to `write_byte`.
    // column zero is the left-most column.
    // note that the unit here is in 8 (or 6)-bit bytes, not pixels.
    //
    // changing the word-width via `set_word_length` will affect the unit.
    //
    // the internal counter dictates how the column is incremented after a write.
    // see `set_counter_direction` for more information.
    //
    // command: SYE
    //
    // cost: one bus operation
    void set_column(u8 column)
    {
        this->write_instruction(0b00100000 | (column & 0b00011111));
    }

    // set the row coordinate for subsequent calls to `write_byte`.
    // row zero is the top-most row.
    // unit: pixels.
    //
    // the internal counter dictates how the row is incremented after a write.
    // see `set_counter_direction` for more information.
    //
    // command: SXE
    //
    // cost: one bus operation
    void set_row(u8 row)
    {
        this->write_instruction(0b10000000 | (row & 0b00111111));
    }

    // > This command sets the top row of the LCD screen, irrespective of the current [Y]-address.
    // > For instance, when the Z-address is 32, the top row of the LCD screen is address 32 
    // > of the display RAM, and the bottom row of the LCD screen is address 31 of the display RAM.
    //
    // wb: I believe you use can use this to implement scrolling.
    //
    // command: SZE
    //
    // cost: one bus operation
    void set_z(u8 z)
    {
        this->write_instruction(0b01000000 | (z & 0b00111111));
    }

    // write a word of data to the LCD, left-to-right.
    // MSB is the leftmost pixel, LSB is the rightmost pixel.
    //
    // changing the word-width via `set_word_length` will affect the unit.
    //
    // the internal counter dictates how the address is incremented after a write.
    // see `set_counter_direction` for more information.
    //
    // cost: one bus operation
    void write_word(u8 v)
    {
        this->write_data(v);
    }

    // naive clear of the LCD by writing zeros to all pixels.
    // requires around 860 commands, which takes a noticable amount of time.
    //
    // cost: 900 bus operations
    void clear()
    {
        Direction d = this->counter_direction;
        WordLength wl = this->word_length;

        if (d != Direction::ROW) {
            this->set_counter_direction(Direction::ROW);
        }

        if (wl != WordLength::WORD_LENGTH_8) {
            this->set_word_length(WordLength::WORD_LENGTH_8);
        }

        for (int y = 0; y < ROW_COUNT; y++) {
            this->set_row(y);
            this->set_column(0);
            for (int x = 0; x < (X_COUNT / 8); x++) {
                this->write_word(0b00000000);
            }
        }

        this->set_counter_direction(d);
        this->set_word_length(wl);
    }

    // command: STRD
    //
    // cost: one bus operation
    Status read_status()
    {
        return Status(this->bus_read(ReadMode::READ_STATUS));
    }

    // read a word of data from the current address.
    // write a word of data to the LCD, left-to-right.
    // MSB is the leftmost pixel, LSB is the rightmost pixel.
    //
    // changing the word-width via `set_word_length` will affect the unit.
    //
    // the internal counter dictates how the address is incremented after a read.
    // see `set_counter_direction` for more information.
    //
    // after changing the address, ensure you read a dummy value first:
    //
    // > However, when a data read is executed, the correct data does not appear on the first 
    // > data reading. Therefore, ensure that the T6A04A performs a dummy data read before
    // > reading the actual data. 
    //
    // once the dummy value is read, its ok to read multiple times.
    //
    // command: DARD
    //
    // cost: one bus operation
    u8 read_word()
    {
        return this->bus_read(ReadMode::READ_DATA);
    }

    // turn the given index on/off within the given word.
    static u8 paint_pixel(u8 word, u8 index, bool color) {
        if (color) {
            return word | (0b10000000 >> index);
        } else {
            return word & ~(0b10000000 >> index);
        }
    }

    // naive update of a single pixel at a given (x, y) location.
    //
    // note that this isn't really very fast: it must read the current word and the write it back.
    // if you have RAM to spare, then you should probably maintain a local screen buffer instead.
    // (but this takes 1024 bytes to represent the entire 64x128 display)
    //
    // it takes about 4s to update the entire screen using this routine repeatedly,
    // which is about .6ms/pixel.
    //
    // so, for example, if you're targetting 16ms/frame, thats about 26 pixels.
    //
    // cost: seven bus operations
    void write_pixel(u8 x, u8 y, bool on)
    {
        if (x >= X_COUNT || y >= Y_COUNT) {
            return;
        }

        u8 row = y;
        u8 column = x / 8;
        u8 bit = x % 8;

        u8 existing = this->read_word_at(row, column);

        u8 next = paint_pixel(existing, bit, on);

        if (next != existing) {
            this->write_word_at(row, column, next);
        }
    }

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        if (x < 0 || x >= X_COUNT || y < 0 || y >= Y_COUNT) {
            return;
        }

        this->write_pixel(x, y, color != 0);
    }

    // read the word at the given coordinates.
    //
    // use only if you expect the coordinates to differ from the current adddress,
    // because this routine updates coordinates and handles the dummy read.
    // this is less efficient than sequential reads that rely on the counter.
    //
    // cost: four bus operations
    u8 read_word_at(u8 row, u8 column)
    {
        this->set_row(row);
        this->set_column(column);
        this->read_word(); // dummy
        return this->read_word();
    }

    // write word at the given coordinates.
    //
    // use only if you expect the coordinates to differ from the current adddress,
    // because this routine updates coordinates.
    // this is less efficient than sequential writes that rely on the counter.
    //
    // cost: three bus operations
    void write_word_at(u8 row, u8 column, u8 word)
    {
        this->set_row(row);
        this->set_column(column);
        this->write_word(word);
    }

    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override
    {
        if (w == 0) {
            // zero width line: no pixels.
            return;
        }

        if (w < 0) {
            // enforce w to be positive.
            x = x + w;
            w = -w;
        }

        if (y < 0 || y >= Y_COUNT) {
            // this is off the screen, low or high
            return;
        }

        u8 start_x = x;
        u8 end_x = x + w;

        if (end_x < 0 || start_x >= X_COUNT) {
            // all pixels off the side of the screen
            return;
        }

        if (start_x < 0) {
            // clamp line within the screen
            start_x = 0;
        }

        if (end_x > X_COUNT) {
            // clamp line within the screen
            end_x = X_COUNT;
        }

        u8 row = y;
        // when writing the middle of the line,
        // we rely on the counter to increment horizontally.
        this->set_counter_direction(Direction::ROW);

        if ((start_x / 8) == (end_x / 8)) {
            // all pixels in the same word
            // 00xxxxxx00
            //
            // cost: seven bus operations
            u8 column = start_x / 8;
            u8 left_pixel = start_x % 8;
            u8 right_pixel = end_x % 8;

            u8 word = this->read_word_at(row, column);

            for (u8 i = left_pixel; i < right_pixel; i++) {
                word = paint_pixel(word, i, 0 != color);
                x += 1;
            }

            this->write_word_at(row, column, word);
        } else {
            // multi-word line
            // 00000xxx xxxxxxxx xxx00000
            // 00000000 xxxxxxxx xxx00000
            // 00000xxx xxxxxxxx 00000000
            // 00000000 xxxxxxxx 00000000  (due to off-by-one above)
            //
            // cost: 16 + (#aligned words) bus operations (max: 28 total)

            // unaligned left side
            // 00000xxx ........
            //
            // cost: seven bus operations
            if (0 != start_x % 8) {
                u8 left_column = start_x / 8;
                u8 left_pixel = start_x % 8;

                u8 left_word = this->read_word_at(row, left_column);

                for (u8 i = left_pixel; i < 8; i++) {
                    left_word = paint_pixel(left_word, i, 0 != color);
                    x += 1;
                }

                this->write_word_at(row, left_column, left_word);
            }

            // aligned middle
            // xxxxxxxx
            // cost: two + (#aligned words) bus operations (max: 14 total)
            this->set_row(row);
            this->set_column(x / 8);
            while (x + 8 <= end_x) {
                // we can blindly overwrite the word
                // because all bits will be set.
                //
                // also, we rely on the counter to increment horizontally.
                // due to the end_x being clamped to the screen dimensions,
                // we can assume the counter doesn't wrap to the next line.
                // this is (mostly) where the "fast" comes from.
                this->write_word(0b11111111);
                x += 8;
            }

            // unaligned right side
            // ........ xxx00000
            //
            // cost: seven bus operations
            if (0 != end_x % 8) {
                u8 right_column = end_x / 8;
                u8 right_pixel = end_x % 8;

                u8 right_word = this->read_word_at(row, right_column);

                for (u8 i = 0; i < right_pixel; i++) {
                    right_word = paint_pixel(right_word, i, 0 != color);
                    x += 1;
                }

                this->write_word_at(row, right_column, right_word);
            }
        }
    }
};

#endif // T6A04A_H