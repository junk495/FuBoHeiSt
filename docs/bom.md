# Stückliste (BOM) – vollständig

Stand: vor Schaltplan. Vollständige Liste aller PCB-Bauteile mit Mengen.
**✅ haben** = im Bestand / bereits bestellt; **🛒 kaufen** = noch zu beschaffen.

## Halbleiter / ICs

| Bauteil | Menge | Status |
|---|---|---|
| ESP32-S3-DevKitC-1 (S3-WROOM-1-N16R8) | 1 | ✅ haben |
| DRV8833PWPR / DRV8833CPWPR (HTSSOP-16) | 4 | ✅ bestellt |
| INA219AIDR (SOIC-8) | 1 | ✅ haben |
| MCP602-I/SN (SOIC-8) | 1 | ✅ haben |
| LM393DR2G (SOIC-8) | 1 | ✅ bestellt |
| ME6217C33M5G (SOT-23-5) | 1 | ✅ haben |
| AO3400A (SOT-23, N-MOSFET) | 2 | ✅ haben (SSR- + Buzzer-Treiber) |
| SSR (3,3 V, 40 A) | 1 | ✅ haben |

## Shunts

| Bauteil | Menge | Status |
|---|---|---|
| 1 Ω 0805 (1 %) | 4 | ✅ haben (2× INA219 + 2× ISEN, je 0,5 Ω) |

## Widerstände (0805, 1 %)

| Wert | Menge | Verwendung |
|---|---|---|
| 1 kΩ | 5 | R1, R2, R5 + 2× AO3400A-Gate-Vorwiderstand |
| 10 kΩ | 6 | R4, R8 (LM393-Pullup), R9 (nSLEEP), R21 (nFAULT) + 2× AO3400A-GS-Pulldown |
| 22 kΩ | 2 | R3 (Gain), Rbias (Slicer-Offset) |
| 4,7 kΩ | 3 | 2× I²C-Pullup + 1-Wire-Pullup |
| 470 kΩ | 1 | Rhyst (Hysterese) |

## Kondensatoren

| Wert | Menge | Verwendung |
|---|---|---|
| 10 nF | 4 | Ladungspumpe VCP→VM (Universal-Footprint für DRV8833 / DRV8833C) |
| 100 nF | 10 | C1 + 3× IC-Abblock + 4× VM-DRV8833 + ME6217-Eingang + Reserve |
| 10 µF | 7 | C2 + 4× VINT + ME6217-Ausgang + C20 (VM-Bulk) |

## Dioden & Schutz

| Bauteil | Menge | Status |
|---|---|---|
| 1N4148W (SOD-123) | 1 | ✅ haben (Buzzer-Freilauf) |
| TVS SMAJ5.0A | 1 | 🛒 kaufen (Überspannungsschutz, optional) |
| PTC-Sicherung 1812L300 | 1 | ✅ haben |

## Verbinder & Sonstiges

| Bauteil | Menge | Status |
|---|---|---|
| Buzzer MLT-8540H | 1 | ✅ haben |
| Buchsenleiste 1×22 (RM 2,54 mm) | 2 | 🛒 kaufen (DevKitC-1-Steckplatz) |
| XH2.54 2-pol (HX-XH2.54-2PZZ) | 10 | ✅ haben (8× Motor: 7 Heizkreise + 1 Reserve + 5-V-Eingang + SSR-Steuerung) |
| XH2.54 3-pol | 1 | ✅ haben (1-Wire DS18B20) |
| DS18B20-Anlegefühler | 1 | 🛒 kaufen |

## Zusammenfassung – noch zu beschaffen

| Bauteil | Menge | ca. Preis |
|---|---|---|
| Buchsenleiste 1×22 (RM 2,54 mm) | 2 | ~0,5 € |
| TVS SMAJ5.0A | 1 | ~0,3 € (optional) |
| DS18B20-Anlegefühler | 1 | ~3 € |
| externes 5-V-Netzteil | 1 | ~5 € (falls nicht vorhanden) |

## Noch offen

- Raumfühler (6×): Funk-Sensor, Modell offen (siehe `docs/sensorik.md`).
- Bodensensor (0–1×): nur falls Vorlauf > ~32 °C.

