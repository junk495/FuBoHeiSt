# Schaltplan-/Netzlisten-Konzept

Block-/Netzlisten-Konzept für die FuBoHeiSt-Trägerplatine.
Finale Bauteilwerte und exakte Verdrahtung werden im EasyEDA-Schaltplan festgelegt.

## 1. Überblick

- Controller: **ESP32-S3-DevKitC-1** (ESP32-S3-WROOM-1-N16R8) als Steckmodul (U1).
- 7 Motoren über **4× DRV8833** (7 Kanäle + 1 Reserve).
- Strommessung: **INA219** (I²C, Stall) + **Shunt → MCP602 → LM393 → PCNT** (Ripple).
- Hauptventil: **SSR** (3,3 V, 40 A).
- Versorgung: extern 5 V → PCB (**ME6217 (LDO)** → 3,3 V_MOTOR).

## 2. Netzliste (Blöcke)

### 2.1 Versorgung
- Eingang: 5 V (externes Netzteil) + GND; Feinsicherung + TVS am Eingang.
- 5 V → DevKit-Pin „5V" (das DevKit erzeugt sein 3,3 V selbst).
- 5 V → **ME6217 (LDO)** → 3,3 V_MOTOR, mit 100 nF + 10/22 µF Bulk. (LDO statt Schaltregler: kein Schalt-Ripple in der Messkette.)
- 3,3 V_MOTOR → VM aller 4 DRV8833 (parallel). (Der Mess-Shunt liegt Low-Side an AISEN/BISEN, siehe §2.3.)

### 2.2 Motor-Treiber
- 4× DRV8833, je 2 H-Brücken (A/B).
- Kanalzuordnung: Chip1=A/B, Chip2=A/B, Chip3=A/B, Chip4=A/B (1 Reserve).
- IN-Pins an GPIO (siehe §3).
- nSLEEP aller 4 Chips gemeinsam an GPIO18 (HIGH = aktiv).
- nFAULT aller 4 Chips wire-AND + Pullup an GPIO21 (LOW = Fehler).
- Ausgänge → RJ12/6P6C-Buchse, nur die 2 Mitteladern (Pin 3+4).

### 2.3 Strommessung / Ripple
- **INA219** (High-Side) mit **externem Shunt** (100 mΩ aus Bestand) zwischen IN+ (Pin 8) und IN− (Pin 7), I²C an GPIO1/GPIO2 → **DC/Stall**.
  - Hinweis: der INA219 hat **keinen internen Shunt** (den hat erst der INA260) – der Shunt wird extern platziert.
- **Ripple-Abgriff Low-Side:** alle AISEN/BISEN-Pins der 4× DRV8833 → **1 gemeinsamer Shunt** gegen GND: **0,5 Ω (4× 2 Ω parallel)** oder **1 Ω (2× 2 Ω parallel)**.
  - Am ISEN fließt der Strom immer positiv Richtung GND (die H-Brücke richtet intern gleich) → massebezogenes DC+Ripple-Signal, **kein Bias nötig**.
  - **Tiefpass** R1=1 kΩ + C1=100 nF (fc ≈ 1,6 kHz): filtert Bürstenfeuer, lässt Kommutator-Ripple (200–1000 Hz) durch.
- **Stufe 1 – MCP602 (nicht-invertierend, Gain ≈ 23, VDD = 5 V):**
  - IN+ über Tiefpass vom Shunt; IN− an Teiler R2=1 kΩ (GND) / R3=22 kΩ (Feedback).
  - 50 mA → ~575 mV, 120 mA Stall → ~1,38 V am Ausgang (kein Übersteuern).
- **Stufe 2 – LM393 als „Data Slicer" (selbstnachführend, kein DAC/PWM nötig):**
  - IN+ = schnelles Signal (R4=10 kΩ von MCP602-OUT).
  - IN− = gleitender Mittelwert (R5=47 kΩ + C2=1 µF, fc ≈ 3 Hz) → Schwelle folgt dem DC-Pegel automatisch.
  - R_bias=470 kΩ von 3,3 V an IN− (hebt Ruhepegel leicht an, verhindert Schwingen im Stillstand).
  - Hysterese R_hyst=470 kΩ von OUT auf IN+.
  - LM393 = Open-Collector → Pullup 10 kΩ an **3,3 V**; VDD des LM393 = 5 V (Eingangsbereich 0–3,5 V); 2. Kanal fest verdrahten.

### 2.4 SSR (Hauptventil)
- GPIO47 → Vorwiderstand/Transistor → SSR-Eingang (3,3 V, 40 A).
- SSR-Ausgang schaltet 230 V zum Kugelhahn; Snubber/Varistor am Ausgang (induktive Last).

### 2.5 1-Wire
- GPIO40 + 4,7 kΩ Pullup → DS18B20 (Vorlauf) + optionaler Bodenfühler.

## 3. Pin-Belegung (ESP32-S3-DevKitC-1)

| Funktion | GPIO | Footprint-Pin |
|---|---|---|
| Motor 1 IN1/IN2 | 4 / 5 | 4 / 5 |
| Motor 2 IN1/IN2 | 6 / 7 | 6 / 7 |
| Motor 3 IN1/IN2 | 8 / 9 | 12 / 15 |
| Motor 4 IN1/IN2 | 10 / 11 | 16 / 17 |
| Motor 5 IN1/IN2 | 12 / 13 | 18 / 19 |
| Motor 6 IN1/IN2 | 14 / 15 | 20 / 8 |
| Motor 7 IN1/IN2 | 16 / 17 | 9 / 10 |
| nSLEEP (alle DRV8833) | 18 | 11 |
| nFAULT (alle DRV8833) | 21 | 27 |
| SSR (Hauptventil) | 47 | 28 |
| PCNT (Ripple) | 38 | 35 |
| I²C SDA / SCL (INA219) | 1 / 2 | 41 / 40 |
| 1-Wire (Vorlauf/Boden) | 40 | 37 |

Gemieden: Strapping (GPIO0/3/45/46), USB-OTG (19/20), JTAG (39–42), UART0 (43/44), OPI-PSRAM (GPIO33–37, N16R8).

Vollständige Pinbelegung + Hinweise: siehe [`esp32_devkit.md`](esp32_devkit.md).

## 4. Offene Punkte (im Schaltplan zu klären)
- R/C-Werte (Gain, Filter-Grenzfrequenzen) sind Startwerte – nach Messung von Stall-Strom/Ripple finalisieren.
- SSR-Mindestlast prüfen: 40-A-SSR an einem kleinen Kugelhahn (oft < 50 mA) schaltet evtl. nicht sauber → ggf. kleines Relais / kleines 2-A-SSR (z. B. Omron G3MB-202P).
