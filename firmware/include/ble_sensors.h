#pragma once

// BLE-Scan der Raumfühler (BTHome v2) und Temperatur-Aggregation.
namespace BleSensors {

void begin();                      // NimBLE initialisieren
void scan();                       // einmal SCAN_WINDOW_MS scannen (blockierend)
bool hasTemp(int roomIndex);       // letzter Wert vorhanden?
float temp(int roomIndex);         // letzte Temperatur in °C

}  // namespace BleSensors
