#pragma once

// ThingSpeak-Anbindung: zwei getrennte Kanäle (Raumtemperatur + DS18B20).

namespace ThingSpeak {

// Ein Messwert (Temperatur) für ein ThingSpeak-Feld.
struct Field {
    int field;    // Feldnummer (0 = nicht senden)
    float value;  // Messwert
    bool valid;   // gültig?
};

// Sendet die BLE-Raumtemperaturen an den Raumtemperatur-Kanal
// (THINGSPEAK_API_KEY in secrets.h). Rückgabe: true bei HTTP 200.
bool publishRoomTemps(const float temps[], const bool valid[], int count);

// Sendet DS18B20-Werte an den DS18B20-Kanal
// (THINGSPEAK_DS18B20_API_KEY in secrets.h). Rückgabe: true bei HTTP 200.
bool publishDs18b20(const Field fields[], int count);

}  // namespace ThingSpeak
