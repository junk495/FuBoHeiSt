#pragma once

#include <cstdint>

// Kommutator-Ripple zählen über die Hardware-PCNT-Einheit des ESP32-S3.
// Signal: LM393-Ausgang → GPIO38 (steigende Flanke = 1 Puls).
// Richtung kennt die Firmware – hier wird nur gezählt.
class RippleCounter {
public:
    void begin();          // PCNT-Unit konfigurieren + starten
    int16_t count() const; // aktueller Zählerstand
    void clear();          // Zähler auf 0
    void pause();
    void resume();
};
