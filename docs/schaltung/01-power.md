# 01 – Versorgung (Power)

> Zweck: 5-V-Eingang → Schutz (PTC + TVS) → **ME6217-LDO → 3,3 V_MOTOR**.
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
CN9 (5V) ─► F1 (PTC) ─► +5V ─┬─► U9 (ME6217) ─► +3V3_MOTOR ─► (INA219-Shunt) ─► DRV8833 VM
                              ├─► U1 (DevKitC-1, 5V-Pin)
                              ├─► U7 (MCP602, VDD)  ── 5 V
                              ├─► U8 (LM393, VCC)   ── 5 V
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| CN9 | XH2.54 2-pol | THT 2,54 mm | — | 5-V-Eingang |
| F1 | PTC 1812L300 (24 V, 3 A) | 1812 | C20627123 | rücksetzbare Sicherung |
| D2 | TVS SMAJ5.0A | SMA | — | Überspannungsschutz (opt.) |
| U9 | ME6217C33M5G (3,3-V-LDO) | SOT-23-5 | C427602 | 5 V → 3,3 V_MOTOR |
| C18 | 100 nF (X7R) | 0805 | — | U9-Eingang |
| C19 | 10 µF | 0805 | — | U9-Ausgang (Bulk) |

## 3. ME6217 – Pinbelegung (SOT-23-5)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | VIN | Eingang | +5V |
| 2 | GND | Masse | GND |
| 3 | CE | Chip-Enable | +5V (mit VIN verbinden!) |
| 4 | NC | — | offen |
| 5 | VOUT | 3,3 V | +3V3_MOTOR |

## 4. Netzliste

| Netz | Pins |
|---|---|
| 5V_IN | CN9.1, F1.1 |
| +5V | F1.2, U9.1 (VIN), U9.3 (CE), C18.1, U1-5V, U7.8 (VDD), U8.8 (VCC), D2.1 |
| +3V3_MOTOR | U9.5 (VOUT), C19.1 |
| GND | CN9.2, U9.2 (GND), C18.2, C19.2, D2.2 |

## 5. Hinweise

- **CE (Pin 3) an VIN (Pin 1)** klemmen, sonst bleibt 3,3 V_MOTOR dauerhaft aus.
- **LDO statt Schaltregler** für 3,3 V_MOTOR → kein Schalt-Ripple in der analogen Messkette.
- MCP602 (U7) und LM393 (U8) laufen an **+5V** (mehr Eingangs-/Ausgangs-Headroom).

## 6. LCSC-Suchbegriffe

| Bauteil | Suchbegriff | Filter |
|---|---|---|
| LDO 3,3 V | `ME6217` | SOT-23-5 |
| PTC | `1812L 3A` | 1812 |
| TVS 5 V | `SMAJ5.0A` | SMA/DO-214AC |
