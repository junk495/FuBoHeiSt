// FuBoHeiSt – Sensor-Logging + Ventilsteuerung
// Liest 6 BLE-Raumfühler (BTHome v2) aus und pusht die Temperaturen an ThingSpeak.
// Zusätzlich: nicht-blockierende Ventilsteuerung (8 HmIP-VDMOT) über ein serielles
// Kommando-Interface (k<N>, v<N>, i, h).
// WLAN-Zugangsdaten werden über ein Captive-Portal eingegeben und im NVS gespeichert.

#include <Arduino.h>

#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"

#include "ble_sensors.h"
#include "config.h"
#include "ds18b20_config.h"
#include "pins.h"
#include "thingspeak.h"
#include "wifi_config.h"
#include "MotorDriver.h"
#include "CurrentSense.h"
#include "RippleCounter.h"
#include "ValveController.h"

MotorDriver motor;
CurrentSense sense;
RippleCounter ripple;
ValveController valve(motor, sense, ripple);

// DS18B20-Sensoren am 1-Wire-Bus (Pin::ONEWIRE).
// OneWireNg unterstützt auch GPIO > 33 (z. B. GPIO40) auf dem ESP32-S3.
OneWireNg* ow = nullptr;

// Messwerte je konfiguriertem DS18B20 (siehe ds18b20_config.h).
struct DsReading {
    float temp = 0.0f;
    bool valid = false;
    int error = -1;
};
DsReading dsReadings[Ds18b20Config::SENSOR_COUNT];

// Hex-String (16 Hex-Zeichen, optional mit -, :, Leerzeichen getrennt)
// → 8-Byte-ROM-Adresse (OneWireNg::Id). Rückgabe: true bei Erfolg.
bool parseRom(const char* str, OneWireNg::Id& id) {
    uint8_t bytes[8];
    int n = 0;
    int hi = -1;
    for (const char* p = str; *p; p++) {
        const char c = *p;
        int v;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else if (c == '-' || c == ':' || c == ' ' || c == '_') continue;
        else return false;

        if (hi < 0) {
            hi = v;
        } else {
            if (n >= 8) return false;
            bytes[n++] = static_cast<uint8_t>((hi << 4) | v);
            hi = -1;
        }
    }
    if (hi >= 0 || n != 8) return false;
    for (int i = 0; i < 8; i++) id[i] = bytes[i];
    return true;
}

// Liest alle konfigurierten DS18B20-Sensoren (per ROM-Adresse).
void readAllDs18b20() {
    for (int i = 0; i < Ds18b20Config::SENSOR_COUNT; i++) {
        dsReadings[i].valid = false;
        dsReadings[i].error = -1;
    }
    if (!ow) return;

    DSTherm drv(*ow);
    drv.convertTempAll(DSTherm::MAX_CONV_TIME, false);

    for (int i = 0; i < Ds18b20Config::SENSOR_COUNT; i++) {
        OneWireNg::Id id;
        if (!parseRom(Ds18b20Config::SENSORS[i].rom, id)) continue;  // Platzhalter/ungültig
        Placeholder<DSTherm::Scratchpad> scrpd;
        const OneWireNg::ErrorCode ec = drv.readScratchpad(id, scrpd);
        dsReadings[i].error = static_cast<int>(ec);
        if (ec == OneWireNg::EC_SUCCESS) {
            dsReadings[i].temp = scrpd->getTemp2() / 16.0f;
            dsReadings[i].valid = true;
        }
    }
}

// Serielles Kommando-Interface (nicht-blockierend).
bool   valveBusyPrev = false;

void processCommand(const String& line);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println(F("FuBoHeiSt – Sensor-Logging + Ventilsteuerung"));
    Serial.printf("Fühler: %d | Intervall: %lu s | Scanfenster: %lu s\n",
                  Config::ROOM_COUNT,
                  static_cast<unsigned long>(Config::UPDATE_INTERVAL_MS / 1000),
                  static_cast<unsigned long>(Config::SCAN_WINDOW_MS / 1000));

    motor.begin();
    sense.begin();
    ripple.begin();
    valve.begin();
    // 1-Wire-Vorlauffühler (DS18B20); false = interner Pullup aus
    // (der externe 4,7-kΩ-Pullup auf der Platine bleibt maßgeblich).
    ow = new OneWireNg_CurrentPlatform(Pin::ONEWIRE, false);
    Serial.println(F("Ventilsteuerung bereit (h = Hilfe)."));

    WifiConfig::begin();   // WLAN verbinden oder Captive-Portal (nicht-blockierend)
    BleSensors::begin();   // BLE-Scanner vorbereiten

    Serial.println(F("Starte Messzyklus.\n"));
}

void loop() {
    // 1. Serielle Befehle verarbeiten (readStringUntil: verarbeitet Eingaben
    //    auch ohne abschließenden Zeilenumbruch nach kurzem Timeout –
    //    gleiche Logik wie im motor_bringup-Test).
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) processCommand(line);
    }

    // 2. Ventil-Zustandsmaschine + WLAN (beides nicht-blockierend, unabhängig
    //    vom Messzyklus; Ventil und serielle Befehle funktionieren auch ohne WLAN)
    valve.tick();
    WifiConfig::tick();

    // Abschluss der letzten Ventil-Operation melden
    if (valveBusyPrev && !valve.busy()) {
        const ValveController::Result r = valve.result();
        Serial.printf("Ventil fertig: ok=%d stall=%d ziel=%d pulse=%d\n",
                      r.ok, r.stalled, r.targetReached, r.pcnt);
    }
    valveBusyPrev = valve.busy();

    // 3. BLE/ThingSpeak – nicht-blockierend: Der Scan läuft im Hintergrund
    //    (NimBLE-Task); loop() bleibt währenddessen für WLAN-Reconnect und
    //    serielle Befehle bedienbar. Ventil hat weiterhin Vorrang.
    static bool firstRun = true;
    static uint32_t lastCycle = 0;
    static bool scanActive = false;
    const uint32_t now = millis();

    if (valve.busy()) {
        delay(1);   // Ventil aktiv: nur ticken, kein neuer Scan-Zyklus
        return;
    }

    if (!scanActive) {
        if (!firstRun && now - lastCycle < Config::UPDATE_INTERVAL_MS) {
            delay(10);
            return;
        }
        firstRun = false;
        lastCycle = now;

        Serial.println(F("--- Scan ---"));
        if (BleSensors::startScan()) {
            scanActive = true;
        } else {
            Serial.println(F("Scan konnte nicht gestartet werden."));
        }
        return;
    }

    // Scan läuft: nur ticken, Auswertung folgt nach Scan-Ende.
    if (BleSensors::isScanning()) {
        delay(10);
        return;
    }

    // Scan beendet → Ergebnisse auslesen und senden.
    scanActive = false;

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

    // DS18B20-Temperaturen (1-Wire) lesen und ausgeben
    readAllDs18b20();
    for (int i = 0; i < Ds18b20Config::SENSOR_COUNT; i++) {
        if (dsReadings[i].valid) {
            Serial.printf("  %-8s %.1f °C\n", Ds18b20Config::SENSORS[i].name, dsReadings[i].temp);
        } else {
            Serial.printf("  %-8s (kein Wert)\n", Ds18b20Config::SENSORS[i].name);
        }
    }

    // ThingSpeak: Raumtemperaturen (Feld 1-6) + DS18B20 (Feld != 0)
    ThingSpeak::Field extra[Ds18b20Config::SENSOR_COUNT];
    int extraCount = 0;
    for (int i = 0; i < Ds18b20Config::SENSOR_COUNT; i++) {
        if (Ds18b20Config::SENSORS[i].field == 0) continue;
        extra[extraCount].field = Ds18b20Config::SENSORS[i].field;
        extra[extraCount].value = dsReadings[i].temp;
        extra[extraCount].valid = dsReadings[i].valid;
        extraCount++;
    }

    ThingSpeak::publishTemps(temps, valid, Config::ROOM_COUNT, extra, extraCount);
}

void processCommand(const String& line) {
    const char c = line[0];

    if (c == 'w') {
        if (line == "wr") {
            WifiConfig::resetCredentials();
        } else {
            WifiConfig::printStatus();
        }
        return;
    }

    if (c == 't') {
        readAllDs18b20();
        for (int i = 0; i < Ds18b20Config::SENSOR_COUNT; i++) {
            if (dsReadings[i].valid) {
                Serial.printf("%s: %.1f °C\n", Ds18b20Config::SENSORS[i].name, dsReadings[i].temp);
            } else {
                Serial.printf("%s: (kein Wert, Fehler %d)\n",
                              Ds18b20Config::SENSORS[i].name, dsReadings[i].error);
            }
        }
        return;
    }

    if (c == 's') {
        Serial.println(F("--- DS18B20 am Bus (Adressen) ---"));
        if (!ow) { Serial.println(F("  (1-Wire nicht initialisiert)")); return; }
        int n = 0;
        ow->searchReset();
        OneWireNg::Id id;
        while (ow->search(id) == OneWireNg::EC_MORE) {
            n++;
            Serial.print(F("  "));
            for (int i = 0; i < 8; i++) {
                if (i) Serial.print('-');
                Serial.printf("%02x", static_cast<unsigned>(id[i]));
            }
            Serial.println();
        }
        Serial.printf("  Gesamt: %d Sensor(en)\n", n);
        return;
    }

    if (c == 'h') {
        Serial.println(F("--- Befehle ---"));
        Serial.println(F("  k<N>  Ventil N kalibrieren (Referenzlauf, Hub speichern)"));
        Serial.println(F("  v<N> [0-100|a|z]  Ventil N auf X% (a=auf, z=zu), ohne Arg = Position"));
        Serial.println(F("  i     Status aller Ventile"));
        Serial.println(F("  t     DS18B20-Temperaturen lesen"));
        Serial.println(F("  s     DS18B20-Adressen suchen (für Konfiguration)"));
        Serial.println(F("  w     WLAN-Status"));
        Serial.println(F("  wr    WLAN-Daten löschen → Portal"));
        Serial.println(F("  h     Hilfe"));
        return;
    }

    if (c == 'i') {
        Serial.println(F("Kanal  Position  Ref  Hub"));
        for (int ch = 0; ch < 8; ch++) {
            Serial.printf("  %d   %6ld   %c   %lu\n", ch,
                          static_cast<long>(valve.position(ch)),
                          valve.hasReference(ch) ? 'j' : 'n',
                          static_cast<unsigned long>(valve.stroke(ch)));
        }
        return;
    }

    if (c == 'k') {
        const int ch = line.substring(1).toInt();
        if (ch < 0 || ch > 7) { Serial.println(F("Kanal 0..7")); return; }
        if (valve.calibrate(ch)) Serial.printf("Kalibriere Ventil %d...\n", ch);
        else Serial.println(F("Ventilsteuerung beschaefigt"));
        return;
    }

    if (c == 'v') {
        // Kanal-Ziffern nach 'v' extrahieren
        int i = 1;
        while (i < line.length() && line[i] >= '0' && line[i] <= '9') i++;
        if (i == 1) { Serial.println(F("Kanal fehlt (v<N>)")); return; }
        const int ch = line.substring(1, i).toInt();
        if (ch < 0 || ch > 7) { Serial.println(F("Kanal 0..7")); return; }

        String arg = line.substring(i);
        arg.trim();

        if (arg.length() == 0) {
            Serial.printf("v%d: pos=%ld/%lu pulse (ref=%c)\n", ch,
                          static_cast<long>(valve.position(ch)),
                          static_cast<unsigned long>(valve.stroke(ch)),
                          valve.hasReference(ch) ? 'j' : 'n');
        } else if (arg == "a") {
            if (valve.open(ch)) Serial.printf("v%d: auf...\n", ch);
            else Serial.println(F("beschaeftigt"));
        } else if (arg == "z") {
            if (valve.close(ch)) Serial.printf("v%d: zu...\n", ch);
            else Serial.println(F("beschaeftigt"));
        } else {
            const int pct = arg.toInt();
            if (pct < 0 || pct > 100) { Serial.println(F("0..100, a oder z")); return; }
            if (valve.moveTo(ch, pct)) Serial.printf("v%d -> %d %%...\n", ch, pct);
            else Serial.println(F("nicht kalibriert oder beschaefigt"));
        }
        return;
    }

    Serial.printf("Unbekannt: '%s' (h = Hilfe)\n", line.c_str());
}
