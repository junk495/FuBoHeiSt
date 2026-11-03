#pragma once

// Pin-Belegung FuBoHeiSt – exakt nach docs/schaltung/07-controller.md
#include <cstdint>

namespace Pin {

// I²C (INA219, Adresse 0x40)
constexpr uint8_t I2C_SDA = 1;
constexpr uint8_t I2C_SCL = 2;

// 8 Motor-Kanäle (4× DRV8833): Index 0..7 = M1..M8, je {IN1, IN2}
constexpr uint8_t MOTOR_COUNT = 8;
constexpr uint8_t MOTOR_IN[MOTOR_COUNT][2] = {
    {4,  5},   // M1 – Zone 1 (U2-A)
    {6,  7},   // M2 – Zone 2 (U2-B)
    {8,  9},   // M3 – Zone 3 (U3-A)
    {10, 11},  // M4 – Zone 4 (U3-B)
    {12, 13},  // M5 – Zone 5 (U4-A)
    {14, 15},  // M6 – Zone 6 (U4-B)
    {16, 17},  // M7 – Zone 7 (U5-A)
    {41, 42},  // M8 – Reserve (U5-B)
};

// DRV8833 – global
constexpr uint8_t MOTOR_nSLEEP = 18;   // HIGH = aktiv
constexpr uint8_t MOTOR_nFAULT = 21;   // LOW = Fehler (open-drain, wire-AND)

// Ripple-Messung: LM393-Ausgang → PCNT
constexpr uint8_t RIPPLE_PCNT = 38;

// Buzzer (passiv, Rechtecksignal über Q2)
constexpr uint8_t BUZZER = 39;

// 1-Wire (DS18B20-Anlegefühler)
constexpr uint8_t ONEWIRE = 40;

// SSR (Hauptventil, 230 V)
constexpr uint8_t SSR_CTRL = 47;

} // namespace Pin
