// FuBoHeiSt – Bringup-/Kalibrier-Modus (Stufe 1)
// Serielles Kommando-Interface zum Testen der Hardware:
//   c<N>  Kanal wählen (1..8),  f = vorwärts,  r = rückwärts,  s = Brake,
//   i     INA219-Status,        b = Buzzer-Piep, t = DS18B20,  h = Hilfe.
//   m<Wert> Stall-Schwelle setzen (mA), m = anzeigen.
//   o<Wert> Shunt-Widerstand setzen (mOhm), o = anzeigen.
//   a<N>   Automatik: N× (r→Endlage, f→Endlage), Log in NVS
//   d      Log aus NVS auslesen
//   k<N>   Ventil N kalibrieren (Referenzlauf, Hub speichern)
//   v<N> <0-100>|a|z  Ventil N auf X% schließen (a=auf, z=zu)
// Ziel: P_touch, P_ges und die Stall-Schwelle am echten HmIP-VDMOT ermitteln.

#include <Arduino.h>
#include "pins.h"
#include "MotorDriver.h"
#include "CurrentSense.h"
#include "RippleCounter.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

MotorDriver motor;
CurrentSense sense;
RippleCounter ripple;
OneWire oneWire(Pin::ONEWIRE);
DallasTemperature dallas(&oneWire);

uint8_t currentChannel = 0;              // 0 = Kanal 1 (Zone 1)
float stallThresholdMa = 50.0f;          // Lauf ~17 mA, r-Endlage ~79 mA, f-Endlage (Dichtung) ~61 mA → 50 mA
const unsigned long moveTimeoutMs   = 15000;
const unsigned long stallDebounceMs = 100;
const unsigned long stallBlankMs     = 400;   // Blindzeit: Anlaufstrom (Inrush) ignorieren
const unsigned long sampleIntervalMs = 10;   // Abtastung für die Stall-Erkennung
const unsigned long printIntervalMs  = 500;  // Monitor-Ausgabe (nicht jeden Sample)
const unsigned long startWatchdogMs  = 1000; // Start-Watchdog: ohne Lauf nach dieser Zeit → Retry
const float        startWatchdogMa   = 10.0f;// unter diesem Strom gilt der Motor als "nicht gelaufen"
const int          maxStartRetries   = 3;    // maximale Ansteuer-Versuche

struct DriveResult {
    int   pcnt    = 0;    // PCNT-Endwert
    bool  stalled = false;// true = Endlage (Stall) erreicht, false = Timeout/Abbruch
    float stallMa = 0.0f; // Strom bei Stall-Erkennung
    bool  targetReached = false; // true = Ziel-Pulse erreicht
};

const unsigned long calTimeoutMs = 60000;  // Automatik: großzügiger Timeout (Ventil belastet)

// Ventil-Anschläge: r = Kolben-Reverse-Endlage = Ventil AUF (hart),
//                  f = Kolben-Forward-Endlage = Ventil ZU (Gummidichtung).
const float OPEN_STALL_MA  = 60.0f;   // r-Endlage (hart): Ventil AUF
const float CLOSE_STALL_MA = 50.0f;   // f-Endlage (Dichtung): Ventil ZU

// Automatik-Modus: Anzahl Durchläufe, die nach dem Boot ohne Serieleingabe
// automatisch starten (für den Betrieb am Ventil ohne Monitor).
// 0 = deaktiviert (nur manuell über den Befehl a<N>). Für den Ventillauf z. B. auf 5 setzen.
#define AUTO_RUN_COUNT 0
#define AUTO_RUN_WAIT_S 20

void printHelp();
void printStatus();
DriveResult runMove(MotorState dir, const char* name, bool verbose = true, unsigned long timeoutMs = moveTimeoutMs, String* seriesOut = nullptr);
void runStandard(int runs);
DriveResult drive(MotorState dir, unsigned long timeoutMs, float stallMa, int32_t targetPulses, bool verbose, String* seriesOut);
bool valveCalibrate(uint8_t channel);
bool valveOpen(uint8_t channel);
bool valveClose(uint8_t channel);
bool valveMoveTo(uint8_t channel, uint8_t percent);
void readLog();
void beep();
void readTemp();

void setup() {
    Serial.begin(115200);
    delay(1000);  // USB-CDC braucht einen Moment zum Verbinden

    motor.begin();
    sense.begin();
    ripple.begin();
    dallas.begin();

    Serial.println();
    Serial.println(F("FuBoHeiSt – Bringup/Diagnose (Stufe 1)"));
    if (!sense.present()) {
        Serial.println(F("WARNUNG: INA219 (0x40) nicht gefunden - Strommessung/Stall-Erkennung unbrauchbar."));
    }
    printHelp();

#if AUTO_RUN_COUNT > 0
    // Betrieb am Ventil ohne Monitor: nach kurzer Wartezeit automatisch starten.
    // Jede Serieleingabe in dieser Zeit bricht ab (Eingabe bleibt im Puffer).
    Serial.printf("\nAutomatik-Modus: %d Durchläufe in %d s – Eingabe bricht ab.\n", AUTO_RUN_COUNT, AUTO_RUN_WAIT_S);
    unsigned long _t0 = millis();
    bool _auto = true;
    while (millis() - _t0 < AUTO_RUN_WAIT_S * 1000UL) {
        if (Serial.available()) { _auto = false; break; }
        delay(10);
    }
    if (_auto) runStandard(AUTO_RUN_COUNT);
#endif
}

void loop() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    switch (line[0]) {
        case 'c': {
            const int ch = line.substring(1).toInt();
            if (ch >= 1 && ch <= Pin::MOTOR_COUNT) {
                currentChannel = static_cast<uint8_t>(ch - 1);
                Serial.printf("Kanal %d gewählt (IN1=GPIO%d, IN2=GPIO%d)\n",
                              ch, Pin::MOTOR_IN[currentChannel][0], Pin::MOTOR_IN[currentChannel][1]);
            } else {
                Serial.printf("Ungültiger Kanal (1..%d)\n", Pin::MOTOR_COUNT);
            }
            break;
        }
        case 'f': runMove(MotorState::Forward, "vorwärts"); break;
        case 'r': runMove(MotorState::Reverse, "rückwärts"); break;
        case 's': motor.brake(currentChannel); Serial.println(F("Brake")); break;
        case 'm': {
            const String arg = line.substring(1);
            if (arg.length() == 0) {
                Serial.printf("Stall-Schwelle: %.0f mA | Blindzeit %lu ms | Entprellung %lu ms\n",
                              stallThresholdMa, stallBlankMs, stallDebounceMs);
            } else {
                const float v = arg.toFloat();
                if (v > 0.0f) {
                    stallThresholdMa = v;
                    Serial.printf("Stall-Schwelle auf %.0f mA gesetzt\n", stallThresholdMa);
                } else {
                    Serial.println(F("Ungültiger Wert (z. B. m120)"));
                }
            }
            break;
        }
        case 'o': {
            const String arg = line.substring(1);
            if (arg.length() == 0) {
                Serial.printf("Shunt: %.1f mOhm\n", sense.shuntOhm() * 1000.0f);
            } else {
                const float mohm = arg.toFloat();
                if (mohm > 0.0f) {
                    sense.setShuntOhm(mohm / 1000.0f);
                    Serial.printf("Shunt auf %.1f mOhm gesetzt\n", mohm);
                } else {
                    Serial.println(F("Ungültiger Wert (z. B. o50 für 50 mOhm)"));
                }
            }
            break;
        }
        case 'a': {
            const int runs = (line.length() > 1) ? line.substring(1).toInt() : 3;
            if (runs >= 1 && runs <= 10) {
                runStandard(runs);
            } else {
                Serial.println(F("Ungültige Anzahl (1..10)"));
            }
            break;
        }
        case 'd': readLog(); break;
        case 'k': {
            const int ch = line.substring(1).toInt();
            if (ch >= 1 && ch <= Pin::MOTOR_COUNT) {
                valveCalibrate(ch - 1);
            } else {
                Serial.printf("Ungültiger Kanal (1..%d)\n", Pin::MOTOR_COUNT);
            }
            break;
        }
        case 'v': {
            const String rest = line.substring(1);
            const int sp = rest.indexOf(' ');
            if (sp < 0) {
                Serial.println(F("Nutzung: v<N> <0-100>|a|z"));
                break;
            }
            const int ch = rest.substring(0, sp).toInt();
            String arg = rest.substring(sp + 1);
            arg.trim();
            if (ch < 1 || ch > Pin::MOTOR_COUNT) {
                Serial.printf("Ungültiger Kanal (1..%d)\n", Pin::MOTOR_COUNT);
            } else if (arg == "a") {
                valveOpen(ch - 1);
            } else if (arg == "z") {
                valveClose(ch - 1);
            } else {
                const int pct = arg.toInt();
                if (pct >= 0 && pct <= 100) valveMoveTo(ch - 1, pct);
                else Serial.println(F("Prozent 0..100 (oder a/z)"));
            }
            break;
        }
        case 'i': printStatus(); break;
        case 'b': beep(); break;
        case 't': readTemp(); break;
        case 'h':
        default: printHelp(); break;
    }
}

// Kern-Fahrt: bis Stall, Ziel-Pulsen, Timeout oder Abbruch.
// targetPulses < 0 = kein Ziel (nur Stall/Timeout).
DriveResult drive(MotorState dir, unsigned long timeoutMs, float stallMa,
                  int32_t targetPulses, bool verbose, String* seriesOut) {
    DriveResult result;

    if (motor.fault()) {
        if (verbose) Serial.println(F("FEHLER: nFAULT aktiv (Übertemperatur/Überstrom)!"));
        return result;
    }

    motor.enable();
    delay(2);   // DRV8833: Aufwachzeit + Ladungspumpe stabilisieren
    ripple.clear();
    motor.set(currentChannel, dir);

    unsigned long start      = millis();
    unsigned long lastSample = 0;
    unsigned long lastPrint  = 0;
    unsigned long stallSince = 0;
    unsigned long lastDriveMs = start;   // letzter Ansteuer-Versuch (für Start-Watchdog)
    int          startRetries = 0;

    while (true) {
        const unsigned long now = millis();

        if (Serial.available()) {              // manueller Abbruch
            Serial.read();                     // nur das Abbruch-Zeichen verbrauchen, Folge-Kommandos bleiben im Puffer
            if (verbose) Serial.println(F("Abbruch durch Tastendruck"));
            break;
        }
        if (now - start >= timeoutMs) { if (verbose) Serial.println(F("Timeout")); break; }
        if (motor.fault()) {
            if (verbose) Serial.println(F("nFAULT aktiv (OCP/Übertemperatur/UVLO - Ladungspumpe/VM prüfen)"));
            break;
        }

        if (now - lastSample >= sampleIntervalMs) {
            lastSample = now;
            const bool fresh = sense.read();
            const int16_t pulses = ripple.count();

            // Ziel-Pulse erreicht?
            if (targetPulses >= 0 && pulses >= targetPulses) {
                result.targetReached = true;
                if (verbose) Serial.printf("ZIEL: PCNT %d >= %d\n", pulses, targetPulses);
                break;
            }

            if (fresh) {
                const float ma = sense.currentMilliamps();

                // Stall-Erkennung erst nach der Blindzeit: Der Anlaufstrom kann
                // kurzfristig über der Schwelle liegen und würde sonst einen
                // Falsch-Stall auslösen.
                if (now - start >= stallBlankMs) {
                    if (ma > stallMa) {
                        if (stallSince == 0) {
                            stallSince = now;
                        } else if (now - stallSince >= stallDebounceMs) {
                            result.stalled = true;
                            result.stallMa = ma;
                            if (verbose) Serial.printf("STALL: %.1f mA > %.0f mA (entprellt %lu ms)\n",
                                                       ma, stallMa, stallDebounceMs);
                            break;
                        }
                    } else {
                        stallSince = 0;
                    }
                }

                // Start-Watchdog: Läuft der Motor nicht an (Strom bleibt unter der Schwelle)?
                if (startRetries < maxStartRetries &&
                    now - lastDriveMs >= startWatchdogMs &&
                    ma < startWatchdogMa) {
                    if (verbose) Serial.printf("  Start-Watchdog: Motor läuft nicht (I=%.1f mA) → Retry %d/%d\n",
                                               ma, startRetries + 1, maxStartRetries);
                    motor.disable();
                    delay(50);
                    motor.enable();
                    delay(2);
                    motor.set(currentChannel, dir);
                    lastDriveMs = now;
                    startRetries++;
                    stallSince = 0;
                    continue;
                }

                // Alle printIntervalMs (0,5 s) protokollieren/ausgeben –
                // die Stall-Erkennung läuft weiter mit sampleIntervalMs (10 ms).
                if (now - lastPrint >= printIntervalMs) {
                    lastPrint = now;
                    if (verbose) Serial.printf("  t=%lu ms | I=%6.1f mA | PCNT=%6d\n", now - start, ma, pulses);
                    if (seriesOut) *seriesOut += String(pulses) + "," + String(ma, 1) + ";";
                }
            } else if (now - lastPrint >= printIntervalMs) {
                // Kein frischer Messwert – nicht mit veralteten Werten entscheiden.
                lastPrint = now;
                if (verbose) Serial.printf("  t=%lu ms | INA219 n/a | PCNT=%6d\n", now - start, pulses);
                if (seriesOut) *seriesOut += String(pulses) + ",-1;";
            }
        }
    }

    motor.brake(currentChannel);
    motor.disable();              // nSLEEP LOW löscht auch ein gelatchtes nFAULT
    delay(100);
    result.pcnt = ripple.count();
    return result;
}

// Manuelle Einzelfahrt (Befehl f/r): nutzt die globale Stall-Schwelle.
DriveResult runMove(MotorState dir, const char* name, bool verbose, unsigned long timeoutMs, String* seriesOut) {
    if (verbose) Serial.printf("Fahre Kanal %d %s … (Abbruch: Taste | Timeout %lu ms | Stall > %.0f mA)\n",
                               currentChannel + 1, name, timeoutMs, stallThresholdMa);
    DriveResult res = drive(dir, timeoutMs, stallThresholdMa, -1, verbose, seriesOut);
    if (verbose) Serial.printf("Ende: PCNT=%d\n", res.pcnt);
    return res;
}

// ---------- Ventilansteuerung ----------

// Kalibrierten Hub (Pulse) für Kanal lesen. 0 = nicht kalibriert.
int32_t valveStroke(uint8_t channel) {
    Preferences prefs;
    prefs.begin("motorlog", true);
    const int32_t stroke = prefs.getInt(("stroke" + String(channel)).c_str(), 0);
    prefs.end();
    return stroke;
}

// Referenzlauf für Kanal: f→zu (Dichtung), r→auf (Anschlag), Hub messen + speichern.
bool valveCalibrate(uint8_t channel) {
    currentChannel = channel;

    Serial.printf("Kalibriere Kanal %d: f→zu (Dichtung) …\n", channel + 1);
    DriveResult zu = drive(MotorState::Forward, calTimeoutMs, CLOSE_STALL_MA, -1, true, nullptr);
    delay(500);

    Serial.printf("Kalibriere Kanal %d: r→auf (Anschlag) …\n", channel + 1);
    DriveResult auf = drive(MotorState::Reverse, calTimeoutMs, OPEN_STALL_MA, -1, true, nullptr);

    if (!zu.stalled || !auf.stalled) {
        Serial.println(F("Kalibrierung fehlgeschlagen (Endlage nicht erreicht)."));
        return false;
    }

    const int32_t stroke = auf.pcnt;   // Hub = Weg von "zu" bis "auf"
    if (stroke <= 0) {
        Serial.println(F("Kalibrierung fehlgeschlagen (Hub <= 0)."));
        return false;
    }

    Preferences prefs;
    prefs.begin("motorlog", false);
    prefs.putInt(("stroke" + String(channel)).c_str(), stroke);
    prefs.end();

    Serial.printf("Kanal %d kalibriert: Hub = %d Pulse (auf = 0, zu = %d).\n", channel + 1, stroke, stroke);
    return true;
}

// Ventil ganz öffnen (r bis Anschlag).
bool valveOpen(uint8_t channel) {
    currentChannel = channel;
    Serial.printf("Kanal %d: r→auf (Anschlag) …\n", channel + 1);
    DriveResult r = drive(MotorState::Reverse, calTimeoutMs, OPEN_STALL_MA, -1, true, nullptr);
    return r.stalled;
}

// Ventil ganz schließen (f bis Dichtung).
bool valveClose(uint8_t channel) {
    currentChannel = channel;
    Serial.printf("Kanal %d: f→zu (Dichtung) …\n", channel + 1);
    DriveResult r = drive(MotorState::Forward, calTimeoutMs, CLOSE_STALL_MA, -1, true, nullptr);
    return r.stalled;
}

// Ventil auf X % schließen (0 = auf, 100 = zu).
bool valveMoveTo(uint8_t channel, uint8_t percent) {
    currentChannel = channel;

    if (percent == 0) return valveOpen(channel);
    if (percent >= 100) return valveClose(channel);

    const int32_t stroke = valveStroke(channel);
    if (stroke <= 0) {
        Serial.printf("Kanal %d nicht kalibriert – bitte k<%d> ausführen.\n", channel + 1, channel + 1);
        return false;
    }

    const int32_t target = (int32_t)stroke * percent / 100;

    // Erst auf (Referenz 0), dann f bis Ziel.
    if (!valveOpen(channel)) {
        Serial.println(F("Fehler: auf-Referenz nicht erreicht."));
        return false;
    }
    delay(500);

    Serial.printf("Kanal %d: f→%d %% (%d Pulse) …\n", channel + 1, percent, target);
    DriveResult move = drive(MotorState::Forward, calTimeoutMs, CLOSE_STALL_MA, target, true, nullptr);
    return move.targetReached || move.stalled;
}

void runStandard(int runs) {
    Preferences prefs;
    prefs.begin("motorlog", false);
    // alte Einträge löschen
    for (int i = 1; i <= 10; i++) {
        prefs.remove(("run" + String(i)).c_str());
        prefs.remove(("r" + String(i)).c_str());
        prefs.remove(("f" + String(i)).c_str());
    }
    prefs.putInt("count", 0);

    Serial.printf("Starte Automatik: %d Durchläufe (r → Endlage, f → Endlage)\n", runs);

    for (int i = 1; i <= runs; i++) {
        Serial.printf("\n--- Durchlauf %d/%d ---\n", i, runs);

        String rSeries, fSeries;
        DriveResult r = runMove(MotorState::Reverse, "rückwärts", false, calTimeoutMs, &rSeries);
        delay(500);
        DriveResult f = runMove(MotorState::Forward, "vorwärts", false, calTimeoutMs, &fSeries);
        delay(500);

        String s = "r_pcnt:" + String(r.pcnt) +
                   " r_stall:" + String(r.stalled) +
                   " r_ma:" + String(r.stallMa, 1) +
                   " f_pcnt:" + String(f.pcnt) +
                   " f_stall:" + String(f.stalled) +
                   " f_ma:" + String(f.stallMa, 1);
        prefs.putString(("run" + String(i)).c_str(), s);
        prefs.putString(("r" + String(i)).c_str(), rSeries);
        prefs.putString(("f" + String(i)).c_str(), fSeries);

        Serial.printf("  Lauf %d: %s\n", i, s.c_str());
    }

    prefs.putInt("count", runs);
    prefs.end();
    Serial.println(F("\nAutomatik beendet, Log im NVS gespeichert."));
}

void readLog() {
    Preferences prefs;
    prefs.begin("motorlog", true);
    const int count = prefs.getInt("count", 0);
    Serial.printf("Gespeicherte Durchläufe: %d\n", count);
    for (int i = 1; i <= count; i++) {
        String s = prefs.getString(("run" + String(i)).c_str(), "");
        if (s.length() > 0) {
            Serial.printf("\nLauf %d: %s\n", i, s.c_str());
            String r = prefs.getString(("r" + String(i)).c_str(), "");
            String f = prefs.getString(("f" + String(i)).c_str(), "");
            Serial.printf("  r_ts (0,5 s; pcnt,mA): %s\n", r.c_str());
            Serial.printf("  f_ts (0,5 s; pcnt,mA): %s\n", f.c_str());
        }
    }
    prefs.end();
}

void printStatus() {
    const bool present = sense.present();
    const bool fresh = sense.read();
    Serial.println(F("--- Status ---"));
    Serial.printf("  INA219: %s\n", present ? "gefunden" : "NICHT gefunden");
    if (!fresh) Serial.println(F("  Achtung: kein frischer Messwert (I2C-Fehler)"));
    Serial.printf("INA219 0x40 (Shunt %.1f Ω, PGA ÷8 = ±320 mV):\n", sense.shuntOhm());
    const uint16_t cfg = sense.configRegister();
    Serial.printf("  Config: 0x%04X %s\n", cfg, cfg == 0x199F ? "(ok)" : "(FEHLER: Soll 0x199F)");
    Serial.printf("  Shunt-Roh: %d (LSB=10 µV)\n", sense.rawShunt());
    Serial.printf("  Shunt: %.2f mV\n", sense.shuntMillivolts());
    Serial.printf("  Strom: %.2f mA\n", sense.currentMilliamps());
    Serial.printf("  Bus: %.3f V\n", sense.busVolts());
    Serial.printf("nSLEEP: %s\n", motor.isEnabled() ? "aktiv" : "sleep");
    Serial.printf("nFAULT: %s\n", motor.fault() ? "FEHLER (LOW)" : "ok");
    Serial.printf("PCNT: %d\n", ripple.count());
    Serial.println(F("--------------"));
}

void beep() {
    Serial.println(F("Buzzer-Piep (2,7 kHz, 200 ms)"));
    tone(Pin::BUZZER, 2700, 200);
}

void readTemp() {
    dallas.requestTemperatures();
    const float t = dallas.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) {
        Serial.println(F("DS18B20 nicht gefunden"));
    } else {
        Serial.printf("Vorlauf-Temperatur: %.2f °C\n", t);
    }
}

void printHelp() {
    Serial.println(F("--- Befehle ---"));
    Serial.println(F("  c<N>   Kanal wählen (1..8, Default 1)"));
    Serial.println(F("  f      vorwärts bis Stall/Timeout"));
    Serial.println(F("  r      rückwärts bis Stall/Timeout"));
    Serial.println(F("  s      Stopp (Brake)"));
    Serial.println(F("  m<Wert> Stall-Schwelle setzen (mA), m = anzeigen"));
    Serial.println(F("  o<Wert> Shunt-Widerstand setzen (mOhm), o = anzeigen"));
    Serial.println(F("  a<N>   Automatik: N× (r→Endlage, f→Endlage), Log in NVS"));
    Serial.println(F("  d      Log aus NVS auslesen"));
    Serial.println(F("  k<N>   Ventil N kalibrieren (Referenzlauf, Hub speichern)"));
    Serial.println(F("  v<N> <0-100>|a|z  Ventil N auf X% schließen (a=auf, z=zu)"));
    Serial.println(F("  i      INA219-Status + nFAULT/PCNT"));
    Serial.println(F("  b      Buzzer-Piep"));
    Serial.println(F("  t      DS18B20 lesen"));
    Serial.println(F("  h      Hilfe"));
    Serial.println(F("---------------"));
}
