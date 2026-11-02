# 02 – Motor-Treiber (DRV8833 + RJ12)

> Zweck: 7 Ventile (HmIP-VDMOT) über 4× DRV8833 (7 Kanäle + 1 Reserve).
> Ladungspumpe, nSLEEP/nFAULT, Ausgänge zu RJ12 (nur Pin 3+4).
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
GPIO4/5   ─► U2.AIN1/AIN2 ─► U2.AOUT1/AOUT2 ─► CN1 (Zone 1)
GPIO6/7   ─► U2.BIN1/BIN2 ─► U2.BOUT1/BOUT2 ─► CN2 (Zone 2)
GPIO8/9   ─► U3.AIN1/AIN2 ─► U3.AOUT1/AOUT2 ─► CN3 (Zone 3)
GPIO10/11 ─► U3.BIN1/BIN2 ─► U3.BOUT1/BOUT2 ─► CN4 (Zone 4)
GPIO12/13 ─► U4.AIN1/AIN2 ─► U4.AOUT1/AOUT2 ─► CN5 (Zone 5)
GPIO14/15 ─► U4.BIN1/BIN2 ─► U4.BOUT1/BOUT2 ─► CN6 (Zone 6)
GPIO16/17 ─► U5.AIN1/AIN2 ─► U5.AOUT1/AOUT2 ─► CN7 (Zone 7)
GPIO18 ─► nSLEEP (alle)   ·   GPIO21 ◄─ nFAULT (alle, wire-AND)
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| U2–U5 | DRV8833CPWPR | HTSSOP-16 | — | 2 H-Brücken je Chip |
| C3–C6 | 10 µF | 0805 | — | VINT (1 je Chip) |
| C7–C14 | 10 nF | 0603 | — | Ladungspumpe (2 je Chip) |
| R9 | 10 kΩ | 0805 | — | nSLEEP-Pulldown |
| CN1–CN7 | RJ12 6P6C | Würth | 615006138421 | Ventil-Anschluss |

## 3. DRV8833 – Pinbelegung (HTSSOP-16)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | nSLEEP | Enable (HIGH=aktiv) | GPIO18 + R9→GND |
| 2 | AOUT1 | Brücke A, Ausgang 1 | RJ12 Pin 3 |
| 3 | AISEN | Sense A | ISEN |
| 4 | AOUT2 | Brücke A, Ausgang 2 | RJ12 Pin 4 |
| 5 | BOUT2 | Brücke B, Ausgang 2 | RJ12 Pin 4 |
| 6 | BISEN | Sense B | ISEN |
| 7 | BOUT1 | Brücke B, Ausgang 1 | RJ12 Pin 3 |
| 8 | nFAULT | Fehler (LOW) | GPIO21 (wire-AND + Pullup) |
| 9 | AIN1 | Eingang A1 | GPIO |
| 10 | AIN2 | Eingang A2 | GPIO |
| 11 | VINT | 1,8-V-Regler | 10 µF → GND |
| 12 | VM | Motorversorgung | VM (nach INA219-Shunt) |
| 13 | GND | Masse | GND |
| 14 | VCP | Ladungspumpe | 10 nF → VM |
| 15 | CP2 | Ladungspumpe | 10 nF → CP1 |
| 16 | CP1 | Ladungspumpe | 10 nF → CP2 |
| 17 | PowerPAD | Masse | GND (Thermal-Vias) |

## 4. Kanal-Zuordnung

| Chip | Kanal | Zone | IN1/IN2 (GPIO) | Ausgang → |
|---|---|---|---|---|
| U2 | A | 1 | 4 / 5 | CN1 |
| U2 | B | 2 | 6 / 7 | CN2 |
| U3 | A | 3 | 8 / 9 | CN3 |
| U3 | B | 4 | 10 / 11 | CN4 |
| U4 | A | 5 | 12 / 13 | CN5 |
| U4 | B | 6 | 14 / 15 | CN6 |
| U5 | A | 7 | 16 / 17 | CN7 |
| U5 | B | — (Reserve) | BIN1/BIN2 → GND | — |

## 5. Netzliste

| Netz | Pins |
|---|---|
| VM | U2.12, U3.12, U4.12, U5.12, U2.14, U3.14, U4.14, U5.14 (VCP) |
| GND | U2.13, U3.13, U4.13, U5.13, U2.17–U5.17 (PowerPAD), C3.2, C4.2, C5.2, C6.2 |
| nSLEEP | U2.1, U3.1, U4.1, U5.1, R9.1 |
| nFAULT | U2.8, U3.8, U4.8, U5.8 |
| ISEN | U2.3, U2.6, U3.3, U3.6, U4.3, U4.6, U5.3, U5.6 |
| M1_A / M1_B | U2.2 / U2.4 → CN1.3 / CN1.4 |
| M2_A / M2_B | U2.7 / U2.5 → CN2.3 / CN2.4 |
| M3_A / M3_B | U3.2 / U3.4 → CN3.3 / CN3.4 |
| M4_A / M4_B | U3.7 / U3.5 → CN4.3 / CN4.4 |
| M5_A / M5_B | U4.2 / U4.4 → CN5.3 / CN5.4 |
| M6_A / M6_B | U4.7 / U4.5 → CN6.3 / CN6.4 |
| M7_A / M7_B | U5.2 / U5.4 → CN7.3 / CN7.4 |

## 6. Hinweise

- **PowerPAD (Pin 17) zwingend an GND** (4–6 Thermal-Vias).
- nSLEEP (GPIO18) mit **10 kΩ Pulldown** → Treiber bleiben beim ESP32-Boot deaktiviert.
- nFAULT (GPIO21) aller Chips wire-AND (open-drain) + Pullup.
- Reserve-Kanal (U5-B): BIN1/BIN2 fest auf GND.
- RJ12: nur die **mittleren Kontakte Pin 3 + 4** belegen.
