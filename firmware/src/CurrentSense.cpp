#include "CurrentSense.h"
#include "pins.h"
#include <Wire.h>

namespace {
constexpr uint8_t  INA219_ADDR = 0x40;   // A0 = A1 = GND
constexpr uint8_t  REG_CONFIG  = 0x00;
constexpr uint8_t  REG_SHUNT   = 0x01;
constexpr uint8_t  REG_BUS     = 0x02;

// BRNG = 16 V (00), PG = ÷4 (±160 mV, 10), BADC/SADC = 12-Bit (0x3),
// MODE = Shunt+Bus continuous (111)  → 0x119F
constexpr uint16_t CONFIG_VALUE = 0x119F;

constexpr float SHUNT_OHM          = 0.5f;  // 2× 1 Ω parallel
constexpr float SHUNT_LSB_MICROVOLT = 10.0f; // LSB Shunt-Register = 10 µV
}

void CurrentSense::begin() {
    Wire.begin(Pin::I2C_SDA, Pin::I2C_SCL, 100000);
    writeReg(REG_CONFIG, CONFIG_VALUE);
}

bool CurrentSense::present() const {
    Wire.beginTransmission(INA219_ADDR);
    return Wire.endTransmission() == 0;
}

bool CurrentSense::read() {
    if (!present()) return false;
    shuntRaw_ = static_cast<int16_t>(readReg(REG_SHUNT));
    busRaw_   = readReg(REG_BUS);
    return true;
}

int16_t CurrentSense::rawShunt() const { return shuntRaw_; }

float CurrentSense::shuntMillivolts() const {
    return static_cast<float>(shuntRaw_) * SHUNT_LSB_MICROVOLT / 1000.0f;
}

float CurrentSense::currentMilliamps() const {
    // I = U_shunt / R_shunt
    return shuntMillivolts() / SHUNT_OHM;
}

float CurrentSense::busVolts() const {
    // Bus-Register: obere 13 Bit, LSB = 4 mV (16-V-Range)
    return static_cast<float>(busRaw_ >> 3) * 0.004f;
}

void CurrentSense::writeReg(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(INA219_ADDR);
    Wire.write(reg);
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value & 0xFF));
    Wire.endTransmission();
}

uint16_t CurrentSense::readReg(uint8_t reg) const {
    Wire.beginTransmission(INA219_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(static_cast<uint16_t>(INA219_ADDR), static_cast<uint8_t>(2));
    if (Wire.available() < 2) return 0;
    const uint16_t hi = Wire.read();
    const uint16_t lo = Wire.read();
    return (hi << 8) | lo;
}
