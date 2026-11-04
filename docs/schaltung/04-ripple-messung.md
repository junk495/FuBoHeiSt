# 04 – Ripple-Messung (ISEN + MCP602 + LM393)

> Zweck: Kommutator-Ripple zählen für die Positionsbestimmung.
> Low-Side-Shunt → Tiefpass → MCP602 (Gain 23) → LM393 (Data Slicer) → PCNT.
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
ISEN (alle AISEN/BISEN) ─►[R19∥R20 = 0,5 Ω]─► GND
   │
   └─► R1 (1k) + C1 (100n) ─► U7.3 (IN1+) ─► MCP602 (Gain 23) ─► U7.1 (OUT1)
                                    │                              │
                              R2 (1k)→GND, R3 (22k) Feedback      │
                                    │                              ├─► R4 (10k) ─► U8.3 (IN+)
                                    │                              └─► R5 (1k) ─► U8.2 (IN−) + C2 (10µ)→GND + R6 (22k)→+3V3
                                    │                                                U8.1 (OUT) ─ R8 (10k)→+3V3, R7 (470k)→U8.3
                                    │                                                              └─► RIPPLE (GPIO38)
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| R19, R20 | 1 Ω (1 %), 2× parallel | 0805 | — | ISEN-Shunt 0,5 Ω |
| R1 | 1 kΩ | 0805 | — | Tiefpass |
| C1 | 100 nF | 0805 | — | Tiefpass (fc ≈ 1,6 kHz) |
| U7 | MCP602-I/SN | SOIC-8 | C29605 | Verstärker (Gain 23) |
| R2 | 1 kΩ | 0805 | — | Gain-Teiler |
| R3 | 22 kΩ | 0805 | — | Gain-Feedback |
| C16 | 100 nF | 0805 | — | Abblocken U7 (VDD) |
| U8 | LM393DR2G | SOIC-8 | — | Data Slicer |
| R4 | 10 kΩ | 0805 | — | Slicer „schnell" |
| R5 | 1 kΩ | 0805 | — | Slicer Mittelwert |
| C2 | 10 µF | 0805 | — | Slicer Mittelwert (τ ≈ 10 ms) |
| R6 | 22 kΩ | 0805 | — | Rbias (Offset ~143 mV) |
| R7 | 470 kΩ | 0805 | — | Rhyst (Hysterese) |
| R8 | 10 kΩ | 0805 | — | Pullup (Open-Collector) |
| C17 | 100 nF | 0805 | — | Abblocken U8 (VCC) |

## 3. MCP602 – Pinbelegung (SOIC-8)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | OUT1 | Ausgang 1 | R3, R4, R5 |
| 2 | IN1− | Eingang 1 (−) | R2→GND, R3 |
| 3 | IN1+ | Eingang 1 (+) | R1/C1 (vom ISEN) |
| 4 | VSS | Masse | GND |
| 5 | IN2+ | Kanal B (+) | GND (unbenutzt) |
| 6 | IN2− | Kanal B (−) | OUT2 (Pin 7) |
| 7 | OUT2 | Kanal B Ausgang | IN2− (Pin 6) |
| 8 | VDD | Versorgung | +5V (C16→GND) |

## 4. LM393 – Pinbelegung (SOIC-8)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | 1OUT | Ausgang 1 (Open-Collector) | R8→+3V3, R7, RIPPLE |
| 2 | 1IN− | Eingang 1 (−) | R5, C2→GND, R6→+3V3 |
| 3 | 1IN+ | Eingang 1 (+) | R4, R7 |
| 4 | GND | Masse | GND |
| 5 | 2IN+ | Kanal 2 (+) | GND (unbenutzt) |
| 6 | 2IN− | Kanal 2 (−) | +3V3 (unbenutzt) |
| 7 | 2OUT | Kanal 2 Ausgang | NC (offen) |
| 8 | VCC | Versorgung | +5V (C17→GND) |

## 5. Netzliste

| Netz | Pins |
|---|---|
| ISEN | R19.1, R20.1, R1.1 |
| GND | R19.2, R20.2, C1.2, R2.2, U7.4, U7.5, C16.2, U8.4, U8.5, C2.2, C17.2 |
| TP_FILT | R1.2, C1.1, U7.3 (IN1+) |
| MCP_OUT | U7.1 (OUT1), R3.1, R4.1, R5.1 |
| GAIN_FB | U7.2 (IN1−), R2.1, R3.2 |
| +5V | U7.8 (VDD), U8.8 (VCC), C16.1, C17.1 |
| SLC_FAST | R4.2, U8.3 (IN+), R7.2 |
| SLC_SLOW | R5.2, C2.1, R6.2, U8.2 (IN−) |
| +3V3 | R6.1, R8.1, U8.6 (2IN−) |
| RIPPLE | U8.1 (OUT), R8.2, R7.1 |
| (U7-Kanal B) | U7.6 ↔ U7.7 (Folger) |

## 6. Spannungspegel (Startwerte)

| Punkt | @ 50 mA Lauf | @ 120 mA Stall (blockiert) |
|---|---|---|
| ISEN-Shunt (0,5 Ω) | 25 mV DC, ±5 mV Ripple | 60 mV DC, **0 mV Ripple** (Motor steht) |
| MCP602 OUT1 (Gain 23) | 575 mV DC, ±115 mV Ripple (230 mVpp) | 1,38 V DC, **0 mV Ripple** |
| LM393 IN− (Mittel + Offset) | ~OUT1 + Offset | ~OUT1 + Offset |
| LM393 OUT → PCNT | Rechteck bei Ripple-Frequenz (0/3,3 V) | **konstant, keine Pulse** |

## 7. Hinweise

- **Alle Werte sind Startwerte** (Stall-Strom/Ripple noch nicht gemessen) → auf der ersten Platine nachjustieren (Gain über R3, Slicer-Zeit über R5/C2).
- **Gesamt-Spannungsabfall @ 120 mA Stall:**
  - **DRV8833** (N-Kanal, R_DS(on) ≈ 0,36 Ω): 60 mV (INA219-Shunt) + ~43 mV (DRV8833) + 60 mV (ISEN-Shunt) ≈ **163 mV** → **~3,14 V** am Motor.
  - **DRV8833C** (P-Kanal High-Side, R_DS(on) HS+LS ≈ 1,74 Ω): 60 mV (INA219-Shunt) + ~208 mV (DRV8833C) + 60 mV (ISEN-Shunt) ≈ **328 mV** → **~2,97 V** am Motor.
  - Für den 3,3-V-VDMOT sind beide Werte unkritisch (Laufstrom ~50 mA → deutlich weniger Abfall).
- **MCP602 & LM393 an +5V** (mehr Headroom); **Pullup an +3V3** (PCNT-sicher).
- **MCP602 Kanal B als Spannungsfolger** (OpAmp); **LM393 Kanal 2 NICHT als Folger** (Komparator würde schwingen) → Eingänge fest, Ausgang NC.
- Am ISEN fließt der Strom immer positiv Richtung GND → massebezogenes Signal, **kein Bias** nötig.
- **Gain = 1 + R3/R2 = 23.** 50 mA → 575 mV, 120 mA Stall → 1,38 V.
- Data Slicer: τ = R5·C2 ≈ 10 ms (schnelles Einschwingen beim Anlauf); Offset ~143 mV über der +69-mV-Hysterese.
