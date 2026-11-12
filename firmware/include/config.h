#pragma once

// FuBoHeiSt – nicht-geheime Firmware-Konfiguration (Sensor-Logging).
// Geheimnisse (ThingSpeak-Key) stehen in secrets.h (gitignored).

#include <cstdint>

namespace Config {

// Zyklus: alle UPDATE_INTERVAL_MS wird gescannt und an ThingSpeak gesendet.
constexpr uint32_t UPDATE_INTERVAL_MS = 60000;   // ThingSpeak Free: min. 15 s

// BLE-Scanfenster je Zyklus (WiFi-Koexistenz, siehe docs/sensorik.md §4).
constexpr uint32_t SCAN_WINDOW_MS = 10000;

// Raumfühler: Zone ↔ BLE-MAC ↔ ThingSpeak-Feld (Temperatur).
struct RoomSensor {
    const char* zone;
    const char* mac;
    int field;
};

const RoomSensor ROOMS[] = {
    {"WohnKue", "A4:C1:38:C2:92:A7", 1},
    {"Schlafz", "A4:C1:38:51:E4:5B", 2},
    {"KiZi",    "A4:C1:38:E0:BA:0F", 3},
    {"Bad",     "A4:C1:38:36:FB:F1", 4},
    {"WC",      "A4:C1:38:E9:8B:CC", 5},
    {"Gang",    "A4:C1:38:A1:EE:FD", 6},
};

constexpr int ROOM_COUNT = sizeof(ROOMS) / sizeof(ROOMS[0]);

}  // namespace Config
