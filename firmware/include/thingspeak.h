#pragma once

// ThingSpeak-Anbindung.
namespace ThingSpeak {

// Ein zusätzlicher Messwert (Temperatur) für ein ThingSpeak-Feld.
struct Field {
    int field;    // Feldnummer (0 = nicht senden)
    float value;  // Messwert
    bool valid;   // gültig?
};

// Sendet die Raumtemperaturen als field<N>=<wert> (nur gültige Werte) und
// optional weitere Felder (z. B. DS18B20-Sensoren).
// Rückgabe: true bei HTTP 200.
bool publishTemps(const float temps[], const bool valid[], int count,
                  const Field extra[], int extraCount);

}  // namespace ThingSpeak
