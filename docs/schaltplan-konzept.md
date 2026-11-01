# Schaltplan-/Netzlisten-Konzept

Block-/Netzlisten-Konzept für die FuBoHeiSt-Trägerplatine.
Finale Bauteilwerte und exakte Verdrahtung werden im EasyEDA-Schaltplan festgelegt.

## 1. Überblick

- Controller: **ESP32-S3-DevKitC-1** (ESP32-S3-WROOM-1-N16R8) als Steckmodul (U1).
- 7 Motoren über **4× DRV8833** (7 Kanäle + 1 Reserve).
- Strommessung: **INA219** (I²C, Stall) + **Shunt → MCP6002 → TLV3201 → PCNT** (Ripple).
- Hauptventil: **SSR** (3,3 V, 40 A).
- Versorgung: extern 5 V → PCB (**ME6217/MP2307** → 3,3 V_MOTOR).

## 2. Netzliste (Blöcke)

### 2.1 Versorgung
- Eingang: 5 V (externes Netzteil) + GND; Feinsicherung + TVS am Eingang.
- 5 V → DevKit-Pin „5V" (das DevKit erzeugt sein 3,3 V selbst).
- 5 V → ME6217 (oder MP2307) → 3,3 V_MOTOR, mit 100 nF + 10/22 µF Bulk.
- 3,3 V_MOTOR → Shunt → VM aller 4 DRV8833 (parallel).

### 2.2 Motor-Treiber
- 4× DRV8833, je 2 H-Brücken (A/B).
- Kanalzuordnung: Chip1=A/B, Chip2=A/B, Chip3=A/B, Chip4=A/B (1 Reserve).
- IN-Pins an GPIO (siehe §3).
- nSLEEP aller 4 Chips gemeinsam an GPIO18 (HIGH = aktiv).
- nFAULT aller 4 Chips wire-AND + Pullup an GPIO21 (LOW = Fehler).
- Ausgänge → RJ12/6P6C-Buchse, nur die 2 Mitteladern (Pin 3+4).

### 2.3 Strommessung / Ripple
- INA219 in Serie im Motorstrompfad, I²C an GPIO1/GPIO2 (Stall-/DC-Strom).
- Shunt (Bestückungsoption 0,1/0,5/1 Ω):
  - AC-Pfad: Hochpass → MCP6002 (Gain) → TLV3201 (Komparator, Hysterese) → GPIO38 (PCNT).
  - Komparator-Schwelle: Trimmer oder DAC (tuning-freundlich).

### 2.4 SSR (Hauptventil)
- GPIO47 → Vorwiderstand/Transistor → SSR-Eingang (3,3 V, 40 A).
- SSR-Ausgang schaltet 230 V zum Kugelhahn; Snubber/Varistor am Ausgang (induktive Last).

### 2.5 1-Wire
- GPIO35 + 4,7 kΩ Pullup → DS18B20 (Vorlauf) + optionaler Bodenfühler.

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
| 1-Wire (Vorlauf/Boden) | 35 | 32 |

Gemieden: Strapping (GPIO0/3/45/46), USB-OTG (19/20), JTAG (39–42), UART0 (43/44).

## 4. Offene Punkte (im Schaltplan zu klären)
- Shunt-Wert und OpAmp-Gain (nach Messung von Stall-Strom/Ripple).
- Komparator-Schwelle: Trimmer vs. DAC.
- ME6217 vs. MP2307 final.
