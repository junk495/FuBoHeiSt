#pragma once

#include <cstdint>
#include <time.h>

// MQTT-Anbindung (lokal). Veröffentlicht den Telemetrie-Snapshot auf einem
// Topic (heizung/telemetry) und subscribt die Außentemperatur von einem
// externen Sensor-Broker.
namespace Mqtt {

// Initialisiert beide Clients (Server/Port aus config.h).
bool begin();

// Nicht-blockierender Wartungsaufruf aus loop(): verbindet bei Bedarf (mit
// Backoff), hält beide Verbindungen und veröffentlicht im konfigurierten
// Intervall den flachen Snapshot.
//   ts              = Unix-Zeit (UTC); vor NTP-Sync wird nicht gesendet
//   roomTemps/Valid = Raumtemperaturen (BLE), count = Config::ROOM_COUNT
//   dsTemps/dsValid = DS18B20-Werte, Index = Ds18b20Config::SENSORS
//   posPct/posValid = Ventilstellung je Kanal (0–100 %, 0=auf, 100=zu), count=8
// Rückgabe: true, falls in diesem Aufruf veröffentlicht wurde.
bool tick(time_t ts,
          const float roomTemps[], const bool roomValid[], int roomCount,
          const float dsTemps[], const bool dsValid[], int dsCount,
          const int posPct[], const bool posValid[], int posCount);

// Ist die Publish-Verbindung aufgebaut?
bool connected();

}  // namespace Mqtt
