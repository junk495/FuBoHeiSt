#include "RippleCounter.h"
#include "pins.h"
#include "driver/pulse_cnt.h"

// Nutzt die aktuelle PCNT-API (Arduino-ESP32 3.x / IDF 5.x, driver/pulse_cnt.h).

namespace {
// Glitch-Filter: Pulse kürzer als 1 µs als Störung ignorieren.
constexpr uint32_t GLITCH_FILTER_NS = 1000;

// Zählgrenzen (passend zum int16_t-Rückgabewert der Klasse).
constexpr int COUNT_LOW_LIMIT  = -32768;
constexpr int COUNT_HIGH_LIMIT = 32767;

pcnt_unit_handle_t    unit    = nullptr;
pcnt_channel_handle_t channel = nullptr;
}

void RippleCounter::begin() {
    pcnt_unit_config_t unitCfg = {};
    unitCfg.low_limit  = COUNT_LOW_LIMIT;
    unitCfg.high_limit = COUNT_HIGH_LIMIT;
    pcnt_new_unit(&unitCfg, &unit);

    pcnt_glitch_filter_config_t filterCfg = {};
    filterCfg.max_glitch_ns = GLITCH_FILTER_NS;
    pcnt_unit_set_glitch_filter(unit, &filterCfg);

    pcnt_chan_config_t chanCfg = {};
    chanCfg.edge_gpio_num  = Pin::RIPPLE_PCNT;
    chanCfg.level_gpio_num = -1;   // kein Level-Signal (Richtung kennt die Firmware)
    pcnt_new_channel(unit, &chanCfg, &channel);
    pcnt_channel_set_edge_action(channel,
                                 PCNT_CHANNEL_EDGE_ACTION_INCREASE,  // steigende Flanke → +1
                                 PCNT_CHANNEL_EDGE_ACTION_HOLD);      // fallende Flanke ignorieren
    pcnt_channel_set_level_action(channel,
                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP);

    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    pcnt_unit_start(unit);
}

int16_t RippleCounter::count() const {
    int value = 0;
    pcnt_unit_get_count(unit, &value);
    return static_cast<int16_t>(value);
}

void RippleCounter::clear()  { pcnt_unit_clear_count(unit); }
void RippleCounter::pause()  { pcnt_unit_stop(unit); }
void RippleCounter::resume() { pcnt_unit_start(unit); }
