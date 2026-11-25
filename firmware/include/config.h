#pragma once

// FuBoHeiSt – nicht-geheime Firmware-Konfiguration (Sensor-Logging).
// Geheimnisse (ThingSpeak-Key) stehen in secrets.h (gitignored).

#include <cstdint>

namespace Config {

// Zyklus: alle UPDATE_INTERVAL_MS wird gescannt und an ThingSpeak gesendet.
constexpr uint32_t UPDATE_INTERVAL_MS = 120000;  // 2 min (ThingSpeak Free: min. 15 s)

// BLE-Scanfenster je Zyklus (WiFi-Koexistenz, siehe docs/sensorik.md §4).
constexpr uint32_t SCAN_WINDOW_MS = 10000;

// --- MQTT (lokal, Mosquitto) ---
constexpr const char* MQTT_BROKER_HOST = "192.168.1.144";
constexpr uint16_t MQTT_BROKER_PORT = 1883;
constexpr const char* MQTT_TOPIC = "heizung/telemetry";
constexpr uint32_t MQTT_PUBLISH_INTERVAL_MS = 60000;  // 60 s Snapshot

// --- NTP (für den Unix-Zeitstempel ts im MQTT-Snapshot) ---
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long NTP_GMT_OFFSET_S = 0;  // UTC
constexpr int NTP_DST_OFFSET_S = 0;

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
