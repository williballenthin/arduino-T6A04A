#include "test.h"

//
// demonstrate a few features of the T6A04A driver.
// use a serial connection to verify the output.
//
bool test_T6A04A(T6A04A *lcd)
{
    lcd->init();

    //
    // demonstrate status read
    //
    Status s = lcd->read_status();

    if (s.counter_orientation() != CounterOrientation::ROW_WISE) {
        Serial.println("FAIL: unexpected counter orientation");
        return false;
    }

    if (s.counter_direction() != CounterDirection::INCREMENT) {
        Serial.println("FAIL: unexpected counter direction");
        return false;
    }

    if (s.word_length() != WordLength::WORD_LENGTH_8) {
        Serial.println("FAIL: unexpected word length");
        return false;
    }

    if (!s.is_enabled()) {
        Serial.println("FAIL: unexpected status");
        return false;
    }

    if (s.is_busy()) {
        Serial.println("FAIL: unexpected busy");
        return false;
    }

    //
    // demonstrate writing to lcd memory
    //
    lcd->set_row(1);
    lcd->set_column(1);
    lcd->write_word(0b10101010);
    lcd->write_word(0b11111111);

    //
    // demonstrate reading from lcd memory
    //
    lcd->set_row(1);
    lcd->set_column(1);
    // must read a dummy value after changing an address
    u8 dummy = lcd->read_word();

    if (0b10101010 != lcd->read_word()) {
        Serial.println("FAIL: unexpected value at (1, 1)");
        return false;
    }

    if (0b11111111 != lcd->read_word()) {
        Serial.println("FAIL: unexpected value at (2, 1)");
        return false;
    }

    //
    // demonstrate using Adafruit_GFX functionality
    // (but note there aren't any assertions here).
    //
    lcd->drawLine(0, 0, X_COUNT, Y_COUNT, true);
    lcd->drawLine(X_COUNT, 0, 0, Y_COUNT, true);

    lcd->setCursor(1, 1);
    lcd->setTextColor(1);
    lcd->setTextSize(1);
    lcd->setTextWrap(true);
    lcd->println("Hello, world!");

    //
    // pass
    //
    lcd->clear();
    Serial.println("PASS");

    return true;
}