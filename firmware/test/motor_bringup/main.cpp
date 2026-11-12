// FuBoHeiSt – Bringup-/Kalibrier-Modus (Stufe 1)
// Serielles Kommando-Interface zum Testen der Hardware:
//   c<N>  Kanal wählen (1..8),  f = vorwärts,  r = rückwärts,  s = Brake,
//   i     INA219-Status,        b = Buzzer-Piep, t = DS18B20,  h = Hilfe.
// Ziel: P_touch, P_ges und die Stall-Schwelle am echten HmIP-VDMOT ermitteln.

#include <Arduino.h>
#include "pins.h"
#include "MotorDriver.h"
#include "CurrentSense.h"
#include "RippleCounter.h"
#include <OneWire.h>
#include <DallasTemperature.h>

MotorDriver motor;
CurrentSense sense;
RippleCounter ripple;
OneWire oneWire(Pin::ONEWIRE);
DallasTemperature dallas(&oneWire);

uint8_t currentChannel = 0;              // 0 = Kanal 1 (Zone 1)
float stallThresholdMa = 100.0f;         // Startwert – wird am Ventil kalibriert
const unsigned long moveTimeoutMs   = 15000;
const unsigned long stallDebounceMs = 100;

void printHelp();
void printStatus();
void runMove(MotorState dir, const char* name);
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
    printHelp();
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
        case 'i': printStatus(); break;
        case 'b': beep(); break;
        case 't': readTemp(); break;
        case 'h':
        default: printHelp(); break;
    }
}

void runMove(MotorState dir, const char* name) {
    if (motor.fault()) {
        Serial.println(F("FEHLER: nFAULT aktiv (Übertemperatur/Überstrom)!"));
        return;
    }

    motor.enable();
    ripple.clear();
    motor.set(currentChannel, dir);

    unsigned long start      = millis();
    unsigned long lastSample = 0;
    unsigned long stallSince = 0;

    Serial.printf("Fahre Kanal %d %s … (Abbruch: Taste | Timeout %lu ms | Stall > %.0f mA)\n",
                  currentChannel + 1, name, moveTimeoutMs, stallThresholdMa);

    while (true) {
        const unsigned long now = millis();

        if (Serial.available()) {              // manueller Abbruch
            while (Serial.available()) Serial.read();
            Serial.println(F("Abbruch durch Tastendruck"));
            break;
        }
        if (now - start >= moveTimeoutMs) { Serial.println(F("Timeout")); break; }
        if (motor.fault()) { Serial.println(F("nFAULT aktiv")); break; }

        if (now - lastSample >= 10) {
            lastSample = now;
            sense.read();
            const float ma = sense.currentMilliamps();
            const int16_t pulses = ripple.count();

            if (ma > stallThresholdMa) {
                if (stallSince == 0) {
                    stallSince = now;
                } else if (now - stallSince >= stallDebounceMs) {
                    Serial.printf("STALL: %.1f mA > %.0f mA (entprellt %lu ms)\n",
                                  ma, stallThresholdMa, stallDebounceMs);
                    break;
                }
            } else {
                stallSince = 0;
            }

            Serial.printf("  t=%lu ms | I=%6.1f mA | PCNT=%6d\n", now - start, ma, pulses);
        }
    }

    motor.brake(currentChannel);
    motor.disable();
    delay(100);
    Serial.printf("Ende: PCNT=%d\n", ripple.count());
}

void printStatus() {
    sense.read();
    Serial.println(F("--- Status ---"));
    Serial.println(F("INA219 0x40 (0,5 Ω Shunt, PGA ÷4 = ±160 mV):"));
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
    Serial.println(F("  i      INA219-Status + nFAULT/PCNT"));
    Serial.println(F("  b      Buzzer-Piep"));
    Serial.println(F("  t      DS18B20 lesen"));
    Serial.println(F("  h      Hilfe"));
    Serial.println(F("---------------"));
}
