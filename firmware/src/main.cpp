// FuBoHeiSt – Sensor-Logging (Kalibrier-/Logging-Phase)
// Liest 6 BLE-Raumfühler (BTHome v2) aus und pusht die Temperaturen an ThingSpeak.
// WLAN-Zugangsdaten werden über ein Captive-Portal eingegeben und im NVS gespeichert.

#include <Arduino.h>

#include "ble_sensors.h"
#include "config.h"
#include "thingspeak.h"
#include "wifi_config.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println(F("FuBoHeiSt – Sensor-Logging (BTHome → ThingSpeak)"));
    Serial.printf("Fühler: %d | Intervall: %lu s | Scanfenster: %lu s\n",
                  Config::ROOM_COUNT,
                  static_cast<unsigned long>(Config::UPDATE_INTERVAL_MS / 1000),
                  static_cast<unsigned long>(Config::SCAN_WINDOW_MS / 1000));

    WifiConfig::begin();   // WLAN verbinden oder Captive-Portal (blockiert)
    BleSensors::begin();   // BLE-Scanner vorbereiten

    Serial.println(F("Starte Messzyklus.\n"));
}

void loop() {
    static bool firstRun = true;
    static uint32_t lastCycle = 0;
    const uint32_t now = millis();

    if (!firstRun && now - lastCycle < Config::UPDATE_INTERVAL_MS) {
        delay(100);
        return;
    }
    firstRun = false;
    lastCycle = now;

    Serial.println(F("--- Scan ---"));
    BleSensors::scan();

    float temps[Config::ROOM_COUNT];
    bool valid[Config::ROOM_COUNT];
    for (int i = 0; i < Config::ROOM_COUNT; i++) {
        valid[i] = BleSensors::hasTemp(i);
        temps[i] = valid[i] ? BleSensors::temp(i) : 0.0f;
        if (valid[i]) {
            Serial.printf("  %-8s %.1f °C\n", Config::ROOMS[i].zone, temps[i]);
        } else {
            Serial.printf("  %-8s (kein Wert)\n", Config::ROOMS[i].zone);
        }
    }

    ThingSpeak::publishTemps(temps, valid, Config::ROOM_COUNT);
}
