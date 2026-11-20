// FuBoHeiSt – Sensor-Logging + Ventilsteuerung
// Liest 6 BLE-Raumfühler (BTHome v2) aus und pusht die Temperaturen an ThingSpeak.
// Zusätzlich: nicht-blockierende Ventilsteuerung (8 HmIP-VDMOT) über ein serielles
// Kommando-Interface (k<N>, v<N>, i, h).
// WLAN-Zugangsdaten werden über ein Captive-Portal eingegeben und im NVS gespeichert.

#include <Arduino.h>

#include "ble_sensors.h"
#include "config.h"
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

// Serielles Kommando-Interface (nicht-blockierend, zeichenweise gepuffert).
String serialLine;
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
    Serial.println(F("Ventilsteuerung bereit (h = Hilfe)."));

    WifiConfig::begin();   // WLAN verbinden oder Captive-Portal (nicht-blockierend)
    BleSensors::begin();   // BLE-Scanner vorbereiten

    Serial.println(F("Starte Messzyklus.\n"));
}

void loop() {
    // 1. Serielle Befehle verarbeiten (nicht-blockierend)
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\n' || c == '\r') {
            serialLine.trim();
            if (serialLine.length() > 0) processCommand(serialLine);
            serialLine = "";
        } else {
            serialLine += c;
        }
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

    ThingSpeak::publishTemps(temps, valid, Config::ROOM_COUNT);
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

    if (c == 'h') {
        Serial.println(F("--- Befehle ---"));
        Serial.println(F("  k<N>  Ventil N kalibrieren (Referenzlauf, Hub speichern)"));
        Serial.println(F("  v<N> [0-100|a|z]  Ventil N auf X% (a=auf, z=zu), ohne Arg = Position"));
        Serial.println(F("  i     Status aller Ventile"));
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
