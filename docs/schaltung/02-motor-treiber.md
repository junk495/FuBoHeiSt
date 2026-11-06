# 02 – Motor-Treiber (DRV8833 + XH2.54)

> Zweck: 8 Ventil-Anschlüsse (7 Zonen + 1 Reserve) über 4× DRV8833 (8 Kanäle).
> Ladungspumpe, nSLEEP/nFAULT, Ausgänge zu XH2.54 2-pol (Pin 1+2).
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
GPIO41/42 ─► U5.BIN1/BIN2 ─► U5.BOUT1/BOUT2 ─► CN8 (Reserve)
GPIO18 ─► nSLEEP (alle)   ·   GPIO21 ◄─ nFAULT (alle, wire-AND)
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| U2–U5 | DRV8833PWPR / DRV8833CPWPR | HTSSOP-16 | — | 2 H-Brücken je Chip (Universal-Footprint) |
| C3–C6 | 10 µF | 0805 | — | VINT (1 je Chip) |
| C7–C10 | 10 nF | 0805 | — | Ladungspumpe VCP→VM (Pflicht bei DRV8833, optional/DNP bei DRV8833C) |
| C11–C14 | 100 nF | 0805 | — | VM-Entkopplung (1 je Chip) |
| C20 | 10 µF | 0805 | — | VM-Bulk (nach INA219-Shunt) |
| R9 | 10 kΩ | 0805 | — | nSLEEP-Pulldown |
| R21 | 10 kΩ | 0805 | — | nFAULT-Pullup an +3V3 |
| CN1–CN8 | XH2.54 2-pol | THT 2,54 mm | — | Ventil-Anschluss (7 Zonen + 1 Reserve) |

## 3. DRV8833 – Pinbelegung (HTSSOP-16)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | nSLEEP | Enable (HIGH=aktiv) | GPIO18 + R9→GND |
| 2 | AOUT1 | Brücke A, Ausgang 1 | XH2.54 Pin 1 |
| 3 | AISEN | Sense A | ISEN |
| 4 | AOUT2 | Brücke A, Ausgang 2 | XH2.54 Pin 2 |
| 5 | BOUT2 | Brücke B, Ausgang 2 | XH2.54 Pin 2 |
| 6 | BISEN | Sense B | ISEN |
| 7 | BOUT1 | Brücke B, Ausgang 1 | XH2.54 Pin 1 |
| 8 | nFAULT | Fehler (LOW) | GPIO21 (wire-AND + Pullup) |
| 9 | BIN1 | Eingang B1 | GPIO |
| 10 | BIN2 | Eingang B2 | GPIO |
| 11 | VCP / NC | Ladungspumpe (DRV8833) / NC (DRV8833C) | 10 nF → VM (Pin 12) – Universal-Footprint für beide Chip-Varianten |
| 12 | VM | Motorversorgung | VM (nach INA219-Shunt) + 100 nF → GND |
| 13 | GND | Masse | GND |
| 14 | VINT | Interner 1,8-V-Regler | 10 µF → GND (je Chip, nicht brücken) |
| 15 | AIN2 | Eingang A2 | GPIO |
| 16 | AIN1 | Eingang A1 | GPIO |
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
| U5 | B | Reserve | 41 / 42 | CN8 |

## 5. Netzliste

| Netz | Pins |
|---|---|
| VM | U2.12, U3.12, U4.12, U5.12, C7.2, C8.2, C9.2, C10.2, C11.1, C12.1, C13.1, C14.1, C20.1 |
| GND | U2.13, U3.13, U4.13, U5.13, U2.17–U5.17 (PowerPAD), C3.2, C4.2, C5.2, C6.2, C11.2, C12.2, C13.2, C14.2, R9.2, C20.2 |
| VINT_U2 | U2.14, C3.1 |
| VINT_U3 | U3.14, C4.1 |
| VINT_U4 | U4.14, C5.1 |
| VINT_U5 | U5.14, C6.1 |
| VCP_U2 | U2.11, C7.1 |
| VCP_U3 | U3.11, C8.1 |
| VCP_U4 | U4.11, C9.1 |
| VCP_U5 | U5.11, C10.1 |
| nSLEEP | U2.1, U3.1, U4.1, U5.1, R9.1 |
| nFAULT | U2.8, U3.8, U4.8, U5.8, R21.1 |
| +3V3 | R21.2 |
| ISEN | U2.3, U2.6, U3.3, U3.6, U4.3, U4.6, U5.3, U5.6 |
| M1_IN1 / M1_IN2 | U2.16 / U2.15 |
| M2_IN1 / M2_IN2 | U2.9 / U2.10 |
| M3_IN1 / M3_IN2 | U3.16 / U3.15 |
| M4_IN1 / M4_IN2 | U3.9 / U3.10 |
| M5_IN1 / M5_IN2 | U4.16 / U4.15 |
| M6_IN1 / M6_IN2 | U4.9 / U4.10 |
| M7_IN1 / M7_IN2 | U5.16 / U5.15 |
| M8_IN1 / M8_IN2 | U5.9 / U5.10 |
| M1_A / M1_B | U2.2 / U2.4 → CN1.1 / CN1.2 |
| M2_A / M2_B | U2.7 / U2.5 → CN2.1 / CN2.2 |
| M3_A / M3_B | U3.2 / U3.4 → CN3.1 / CN3.2 |
| M4_A / M4_B | U3.7 / U3.5 → CN4.1 / CN4.2 |
| M5_A / M5_B | U4.2 / U4.4 → CN5.1 / CN5.2 |
| M6_A / M6_B | U4.7 / U4.5 → CN6.1 / CN6.2 |
| M7_A / M7_B | U5.2 / U5.4 → CN7.1 / CN7.2 |
| M8_A / M8_B | U5.7 / U5.5 → CN8.1 / CN8.2 |

## 6. Hinweise

- **PowerPAD (Pin 17) zwingend an GND** (4–6 Thermal-Vias).
- **Ladungspumpe (Universal-Footprint):** Pin 11 ist beim **DRV8833** `VCP` (Pflicht: 1× 10 nF → VM) und beim **DRV8833C** `NC`. Der 10-nF-Footprint (C7–C10) bleibt fest geroutet, damit beide Varianten bestückbar sind – beim `C`-Typ optional/DNP. Zusätzlich **100 nF** von VM (Pin 12) → GND je Chip.
- nSLEEP (GPIO18) mit **10 kΩ Pulldown** → Treiber bleiben beim ESP32-Boot deaktiviert.
- nFAULT (GPIO21) aller Chips wire-AND (open-drain) + Pullup.
- Reserve-Kanal (U5-B): voll verkabelt (GPIO41/42 → BIN1/BIN2, CN8) als Hot-Swap-Reserve – kein fester Heizkreis.
- XH2.54: Pin 1 + 2 (Polarität egal – die Umpolung übernimmt die H-Brücke).
