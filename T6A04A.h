/*
 * Arduino driver for the T6A04A Dot Matrix LCD controller,
 * as used by the TI-83+ calculator.
 * 
 *  LCD Pinout use by TI-83+ to Toshiba T6A04A - 17 pin interface
 *  via: https://gist.github.com/parzivail/12ea33cef02794381a06265ff4ef129e
 *
 *   1  VCC [Fat wire 1]   +5
 *   2  GND [Fat wire 2]   GND
 *   3  RST
 *   4  NC
 *   5  NC
 *   6  STB
 *   7  DI
 *   8  CE
 *   9  D7
 *   10 D6
 *   11 D5
 *   12 D4
 *   13 D3
 *   14 D2
 *   15 D1
 *   16 D0
 *   17 RW
 * 
 * TODO:
 *  - drawFastHLine: pre-read/write sequentially
 *  - drawFastVLine
 *  - drawRect
 */

#ifndef T6A04A_H
#define T6A04A_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

typedef unsigned char u8;
typedef u8 pin;

const u8 Y_COUNT = 64;
// the LCD used by TI-83+ has only 96 pixels horizontally,
// although the driver technically supports up to 128 pixels.
const u8 X_COUNT = 96;

const u8 ROW_COUNT = Y_COUNT;
// can't compute COLUMN_COUNT because this depends on the display word size
// which is configurable between 6 and 8 bits.

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

typedef enum CounterOrientation {
    // counter moves left and right.
    COLUMN_WISE = 1,
    // counter moves up and down.
    ROW_WISE = 2,
} CounterOrientation;

typedef enum CounterDirection {
    // counter moves to the left or down.
    INCREMENT = 1,
    // counter moves to the right or up.
    DECREMENT = 2,
} CounterDirection;

typedef struct CounterConfig {
    CounterOrientation orientation;
    CounterDirection direction;
} CounterConfig;

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

    CounterOrientation counter_orientation() const
    {
        if ((this->inner & 0b00000010) != 0) {
            return CounterOrientation::ROW_WISE;
        } else {
            return CounterOrientation::COLUMN_WISE;
        };
    }

    CounterDirection counter_direction() const
    {
        if ((this->inner & 0b00000001) != 0) {
            return CounterDirection::INCREMENT;
        } else {
            return CounterDirection::DECREMENT;
        };
    }

    CounterConfig counter_config() const
    {
        return CounterConfig {
            this->counter_orientation(),
            this->counter_direction(),
        };
    }
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

    CounterConfig counter_config;
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
            if (OUTPUT == m) {
                digitalWrite(this->rw, RW_WRITE);
            } else if (INPUT == m) {
                digitalWrite(this->rw, RW_READ);
            } else {
                Serial.println("error: unexpected IO mode");
                abort();
            }

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

    // turn the given index on/off within the given word.
    static inline u8 paint_pixel(u8 word, u8 index, bool color) {
        if (color) {
            return word | (0b10000000 >> index);
        } else {
            return word & ~(0b10000000 >> index);
        }
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
          counter_config(CounterConfig { CounterOrientation::ROW_WISE, CounterDirection::INCREMENT }),
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
    }

    void init() {
        this->reset();

        this->set_word_length(WordLength::WORD_LENGTH_8);

        this->enable_display();
        this->set_contrast(48);
        this->set_counter_orientation(CounterOrientation::ROW_WISE);
        this->set_counter_direction(CounterDirection::INCREMENT);
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
    
    void set_counter_config(CounterOrientation o, CounterDirection d)
    {
        if (this->counter_config.direction == d && this->counter_config.orientation == o) {
            return;
        }

        this->counter_config = CounterConfig { o, d };
        u8 command = 0b00000100;

        if (o == CounterOrientation::ROW_WISE) {
            command |= 0b00000010;
        } else if (o == CounterOrientation::COLUMN_WISE) {
            // pass: bit is unset
        } else {
            abort();
        }

        if (d == CounterDirection::INCREMENT) {
            command |= 0b00000001;
        } else if (d == CounterDirection::DECREMENT) {
            // pass: bit is unset
        } else {
            abort();
        }

        this->write_instruction(command);
    }

    void set_counter_orientation(CounterOrientation o)
    {
        this->set_counter_config(o, this->counter_config.direction);
    }

    void set_counter_direction(CounterDirection d)
    {
        this->set_counter_config(this->counter_config.orientation, d);
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
    //
    // this may change the counter config and word length.
    //
    // cost: 796 bus operations
    void clear()
    {
        this->set_counter_config(CounterOrientation::ROW_WISE, CounterDirection::INCREMENT);
        this->set_word_length(WordLength::WORD_LENGTH_8);

        // columns are longer than rows,
        // so clear column-wise.
        for (int x = 0; x < (X_COUNT / 8); x++) {
            this->set_row(0);
            this->set_column(x);
            for (int y = 0; y < Y_COUNT; y++) {
                this->write_word(0b00000000);
            }
        }
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

        u8 next = this->paint_pixel(existing, bit, on);

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

    // TODO: we can make this even faster by pre-reading the line sequentially
    // and then writing it back on sequentially,
    // at the expense of one byte allocation per word touched (max 12 bytes).
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
        this->set_counter_config(CounterOrientation::ROW_WISE, CounterDirection::INCREMENT);

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
                word = this->paint_pixel(word, i, 0 != color);
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
                    left_word = this->paint_pixel(left_word, i, 0 != color);
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
                    right_word = this->paint_pixel(right_word, i, 0 != color);
                    x += 1;
                }

                this->write_word_at(row, right_column, right_word);
            }
        }
    }
};

#endif // T6A04A_H