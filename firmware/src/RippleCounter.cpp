#include "RippleCounter.h"
#include "pins.h"
#include "driver/pcnt.h"

// Hinweis: nutzt die Legacy-PCNT-API (Arduino-ESP32 2.x / IDF 4.4).

namespace {
constexpr pcnt_unit_t    PCNT_UNIT    = PCNT_UNIT_0;
constexpr pcnt_channel_t PCNT_CHANNEL = PCNT_CHANNEL_0;

// Glitch-Filter in APB-Takten (80 MHz → 12,5 ns/Takt). 80 Takte ≈ 1 µs Mindest-Pulslänge.
constexpr uint16_t GLITCH_FILTER_CYCLES = 80;
}

void RippleCounter::begin() {
    pcnt_config_t cfg = {};
    cfg.pulse_gpio_num = Pin::RIPPLE_PCNT;
    cfg.ctrl_gpio_num  = PCNT_PIN_NOT_USED;
    cfg.lctrl_mode     = PCNT_MODE_KEEP;
    cfg.hctrl_mode     = PCNT_MODE_KEEP;
    cfg.pos_mode       = PCNT_COUNT_INC;  // steigende Flanke → +1
    cfg.neg_mode       = PCNT_COUNT_DIS;  // fallende Flanke ignorieren
    cfg.counter_h_lim  = 32767;
    cfg.counter_l_lim  = -32768;
    cfg.unit           = PCNT_UNIT;
    cfg.channel        = PCNT_CHANNEL;

    pcnt_unit_config(&cfg);

    pcnt_set_filter_value(PCNT_UNIT, GLITCH_FILTER_CYCLES);
    pcnt_filter_enable(PCNT_UNIT);

    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);
    pcnt_counter_resume(PCNT_UNIT);
}

int16_t RippleCounter::count() const {
    int16_t value = 0;
    pcnt_get_counter_value(PCNT_UNIT, &value);
    return value;
}

void RippleCounter::clear()  { pcnt_counter_clear(PCNT_UNIT); }
void RippleCounter::pause()  { pcnt_counter_pause(PCNT_UNIT); }
void RippleCounter::resume() { pcnt_counter_resume(PCNT_UNIT); }
