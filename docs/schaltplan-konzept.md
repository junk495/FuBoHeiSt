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
- 3,3 V_MOTOR → 0,5 Ω Shunt (INA219, 2× 1 Ω parallel) → VM aller 4 DRV8833 (parallel). (Der Ripple-Shunt liegt Low-Side an AISEN/BISEN, siehe §2.3.)

### 2.2 Motor-Treiber
- 4× DRV8833, je 2 H-Brücken (A/B).
- Kanalzuordnung: Chip1=A/B, Chip2=A/B, Chip3=A/B, Chip4=A/B (1 Reserve).
- IN-Pins an GPIO (siehe §3).
- nSLEEP aller 4 Chips gemeinsam an GPIO18 (HIGH = aktiv) + **10 kΩ Pull-down an GND** (Treiber bleiben während des ESP32-Boots deaktiviert).
- nFAULT aller 4 Chips wire-AND + Pullup an GPIO21 (LOW = Fehler).
- **Ladungspumpe je DRV8833 (Pflicht):** 10 nF zwischen VCP (Pin 14) und VM (Pin 12), 10 nF zwischen CP1 (Pin 16) und CP2 (Pin 15), 10 µF an VINT (Pin 11) → GND.
- **Reserve-Kanal (Chip 4, Kanal B):** ungenutzte BIN1/BIN2 fest auf GND legen (nicht floaten).
- Ausgänge → RJ12/6P6C-Buchse, nur die 2 Mitteladern (Pin 3+4).

### 2.3 Strommessung / Ripple
- **INA219** (High-Side, SOIC-8) mit **externem Shunt** (0,5 Ω = 2× 1 Ω 0805 parallel) zwischen **IN+ (Pin 1)** und **IN− (Pin 2)** → **DC/Stall** (I²C an GPIO1/GPIO2).
  - SOIC-8-Pinout: Pin 1=IN+, Pin 2=IN−, Pin 3=GND, Pin 4=VS (3,3 V), Pin 5=SCL, Pin 6=SDA, Pin 7=A0, Pin 8=A1 (A0/A1 an GND → Adresse 0x40).
  - Hinweis: der INA219 hat **keinen internen Shunt** (den hat erst der INA260) – der Shunt wird extern platziert.
- **Ripple-Abgriff Low-Side:** alle AISEN/BISEN-Pins der 4× DRV8833 → **1 gemeinsamer Shunt (0,5 Ω = 2× 1 Ω 0805 parallel)** gegen GND.
  - Am ISEN fließt der Strom immer positiv Richtung GND (die H-Brücke richtet intern gleich) → massebezogenes DC+Ripple-Signal, **kein Bias nötig**.
  - **Tiefpass** R1=1 kΩ + C1=100 nF (fc ≈ 1,6 kHz): filtert Bürstenfeuer, lässt Kommutator-Ripple (200–1000 Hz) durch.
- **Stufe 1 – MCP602 (nicht-invertierend, Gain ≈ 23, VDD = 5 V):**
  - IN+ über Tiefpass vom Shunt; IN− an Teiler R2=1 kΩ (GND) / R3=22 kΩ (Feedback).
  - 50 mA → ~575 mV, 120 mA Stall → ~1,38 V am Ausgang (kein Übersteuern).
  - **2. Kanal (OpAmp B) als Spannungsfolger:** IN2− (Pin 6) mit OUT2 (Pin 7) verbinden, IN2+ (Pin 5) an GND.
- **Stufe 2 – LM393 als „Data Slicer" (selbstnachführend, kein DAC/PWM nötig):**
  - IN+ = schnelles Signal (R4=10 kΩ von MCP602-OUT).
  - IN− = gleitender Mittelwert (R5=1 kΩ + C2=10 µF, τ ≈ 10 ms) → Schwelle folgt dem DC-Pegel automatisch, ohne beim Anlauf Pulse zu verschlucken.
  - R_bias=22 kΩ von 3,3 V an IN− → Ruhepegel ~143 mV (sicher über der +69-mV-Hysterese → stabil im Stillstand).
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
| Buzzer (Fehler-Signalton) | 39 | 36 |

Gemieden: Strapping (GPIO0/3/45/46), USB-OTG (19/20), UART0 (43/44), OPI-PSRAM (GPIO33–37, N16R8). JTAG (39–42) frei nutzbar als GPIO (39 = Buzzer, 40 = 1-Wire).

Vollständige Pinbelegung + Hinweise: siehe [`esp32_devkit.md`](esp32_devkit.md).

## 4. Offene Punkte (im Schaltplan zu klären)
- R/C-Werte (Gain, Filter-Grenzfrequenzen) sind Startwerte – nach Messung von Stall-Strom/Ripple finalisieren.
- SSR-Mindestlast prüfen: 40-A-SSR an einem kleinen Kugelhahn (oft < 50 mA) schaltet evtl. nicht sauber → ggf. kleines Relais / kleines 2-A-SSR (z. B. Omron G3MB-202P).
