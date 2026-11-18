#pragma once

#include <cstdint>

// INA219 direkt über I²C (Wire) – kein Adafruit-Treiber.
// Konfiguration: 0,5 Ω Shunt, PGA ÷8 = ±320 mV (für Stall-Erkennung am HmIP-VDMOT).
class CurrentSense {
public:
    void begin();                  // Wire starten + INA219 konfigurieren
    bool present() const;          // INA219 ansprechbar?
    bool read();                   // Shunt- + Bus-Register einmal lesen

    int16_t rawShunt() const;      // Rohwert Shunt-Register (LSB = 40 µV bei PGA ÷4)
    float shuntMillivolts() const; // Shunt-Spannung in mV
    float currentMilliamps() const;// Motorstrom in mA
    float busVolts() const;        // Bus-Spannung in V
    uint16_t configRegister() const; // Konfig-Register lesen (Soll 0x119F)
    void setShuntOhm(float ohm);   // Shunt-Widerstand (Default 0,5 Ω) für mA-Berechnung
    float shuntOhm() const;

private:
    void writeReg(uint8_t reg, uint16_t value);
    uint16_t readReg(uint8_t reg) const;

    int16_t shuntRaw_ = 0;
    uint16_t busRaw_ = 0;
    float shuntOhm_ = 0.5f;
};
