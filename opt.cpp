#include "T6A04A.h"
#include "opt.h"


class Benchmark {
protected:
    // implement these!
    virtual void step(T6A04A *lcd, bool color) = 0;
    virtual char* name() = 0;

public:
    void run(T6A04A *lcd)
    {
        lcd->init();
        lcd->clear();
        Serial.print("measuring: ");
        Serial.print(this->name());
        Serial.print(": ");

        u32 ts0 = millis();

        bool color = true;
        for (u32 i = 0; i < 100; i++) {
            this->step(lcd, color);
            color = !color;
        }

        u32 ts1 = millis();

        Serial.print(float(ts1 - ts0) / 1000.0);
        Serial.print("ms");
        Serial.println("");

        return;
    }
};

//
// naive horizontal line (96px) via write_pixel
// Arduino Uno R3: 6.18ms/line
//
class NaiveHLineBenchmark : public Benchmark {
    virtual char* name() override {
        return "naive hline";
    }
    virtual void step(T6A04A *lcd, bool color) override {
        for (u8 x = 0; x < 96; x++) {
            lcd->write_pixel(x, 0, color);
        }
    }
};

//
// optimized horizontal line (96px) via drawFastHLine
// Arduino Uno R3: 0.12ms/line (50x speedup over naive)
//
class FastHLineBenchmark : public Benchmark {
    virtual char* name() override {
        return "fast hline";
    }
    virtual void step(T6A04A *lcd, bool color) override {
        lcd->drawFastHLine(0, 0, 96, color);
    }
};

// naive vertical line (64px) via write_pixel
// Arduino Uno R3: 4.11ms/line
class NaiveVLineBenchmark : public Benchmark {
    virtual char* name() override {
        return "naive vline";
    }
    virtual void step(T6A04A *lcd, bool color) override {
        for (u8 y = 0; y < 64; y++) {
            lcd->write_pixel(0, y, color);
        }
    }
};

// naive 8x8 px rect at (0, 0) via write_pixel
// Arduino Uno R3: 4.12ms/rect
class NaiveAlignedRectBenchmark : public Benchmark {
    virtual char* name() override {
        return "naive aligned rect";
    }
    virtual void step(T6A04A *lcd, bool color) override {
        for (u8 x = 0; x < 8; x++) {
            for (u8 y = 0; y < 8; y++) {
                lcd->write_pixel(x, y, color);
            }
        }
    }
};

// naive 8x8 px rect at (4, 4) via write_pixel
// Arduino Uno R3: 4.12ms/rect
class NaiveUnalignedRectBenchmark : public Benchmark {
    virtual char* name() override {
        return "naive unaligned rect";
    }
    virtual void step(T6A04A *lcd, bool color) override {
        for (u8 x = 0; x < 8; x++) {
            for (u8 y = 0; y < 8; y++) {
                lcd->write_pixel(x + 4, y + 4, color);
            }
        }
    }
};

static Benchmark *benchmarks[] = {
    new NaiveHLineBenchmark(),
    new FastHLineBenchmark(),
    new NaiveVLineBenchmark(),
    new NaiveAlignedRectBenchmark(),
    new NaiveUnalignedRectBenchmark(),
};

void run_benchmarks(T6A04A *lcd)
{
    for (u8 i = 0; i < sizeof(benchmarks) / sizeof(Benchmark*); i++) {
        benchmarks[i]->run(lcd);
    }
}