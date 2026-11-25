#pragma once

#include <cstdint>
#include <time.h>

// MQTT-Anbindung (lokal, Mosquitto). Veröffentlicht den Telemetrie-Snapshot
// (alle Raumtemperaturen) auf einem Topic (heizung/telemetry).
namespace Mqtt {

// Initialisiert den Client (Server/Port/Topic aus config.h).
bool begin();

// Nicht-blockierender Wartungsaufruf aus loop():
// verbindet bei Bedarf (mit Backoff), hält die Verbindung und veröffentlicht
// im konfigurierten Intervall den Snapshot.
//   ts         = Unix-Zeit (UTC); vor NTP-Sync wird nicht gesendet
//   temps/valid = letzte Raumtemperaturen (BLE)
// Rückgabe: true, falls in diesem Aufruf veröffentlicht wurde.
bool tick(time_t ts, const float temps[], const bool valid[], int count);

// Ist die MQTT-Verbindung aufgebaut?
bool connected();

}  // namespace Mqtt