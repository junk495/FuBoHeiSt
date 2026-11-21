#pragma once

// DS18B20-Sensoren am 1-Wire-Bus (GPIO40).
//
// Hier die eigenen Sensoren eintragen. Die 64-Bit-Adresse (ROM) eines Sensors
// lässt sich über den seriellen Befehl `s` auslesen (Format: 16 Hex-Zeichen,
// z. B. "28ff64a1b2c3d4e5" – auch mit Trennzeichen "28-ff-64-a1-b2-c3-d4-e5").
//
//   name  = Anzeigename/Rolle (nur Monitor-Ausgabe)
//   rom   = 64-Bit-Adresse als Hex-String (16 Hex-Zeichen)
//   field = ThingSpeak-Feld (0 = nicht an ThingSpeak senden)

namespace Ds18b20Config {

struct Sensor {
    const char* name;
    const char* rom;
    int field;
};

const Sensor SENSORS[] = {
    // Den Platzhalter "xxxxxxxxxxxxxxxx" durch die echte Adresse ersetzen
    // (über Befehl `s` auslesbar).
    {"Vorlauf", "xxxxxxxxxxxxxxxx", 7},
    // Weitere Sensoren hier ergänzen, z. B.:
    // {"Boden",  "28ff64a1b2c3d4e5", 8},
};

constexpr int SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);

}  // namespace Ds18b20Config
