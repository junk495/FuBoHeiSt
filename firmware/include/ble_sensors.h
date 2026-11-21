#pragma once

// BLE-Scan der Raumfühler (BTHome v2) und Temperatur-Aggregation.
namespace BleSensors {

void begin();                      // NimBLE initialisieren
bool startScan();                  // SCAN_WINDOW_MS scannen (nicht-blockierend)
bool isScanning();                 // true, solange gescannt wird
bool hasTemp(int roomIndex);       // letzter Wert vorhanden?
float temp(int roomIndex);         // letzte Temperatur in °C

}  // namespace BleSensors
