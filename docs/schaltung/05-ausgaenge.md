# 05 – Ausgänge (SSR + Buzzer)

> Zweck: Hauptventil (SSR, 230 V) und Fehler-Signalton (Buzzer), je über AO3400A.
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
GPIO47 ─► R13 (1k) ─► Q1 (AO3400A) ─► CN10 (SSR-Eingang)
                 R14 (10k) → GND

GPIO39 ─► R15 (1k) ─► Q2 (AO3400A) ─► BZ1 (Buzzer)  [D1 Freilauf]
                 R16 (10k) → GND
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| Q1 | AO3400A (N-MOSFET, 30 V, 5,7 A) | SOT-23 | — | SSR-Treiber |
| Q2 | AO3400A (N-MOSFET, 30 V, 5,7 A) | SOT-23 | — | Buzzer-Treiber |
| R13, R15 | 1 kΩ | 0805 | — | Gate-Vorwiderstand |
| R14, R16 | 10 kΩ | 0805 | — | Gate-Source-Pulldown |
| CN10 | XH2.54 2-pol | THT 2,54 mm | — | SSR-Steuerung |
| BZ1 | MLT-8540H (2,5–4,5 V) | SMD 8,5×8,5 (4 Pads) | C95298 | Fehler-Signalton |
| D1 | 1N4148W | SOD-123 | C7420318 | Buzzer-Freilauf |
| SSR | 3,3 V, 40 A (extern) | — | — | Hauptventil (230 V) |

## 3. Netzliste

| Netz | Pins |
|---|---|
| SSR_CTRL | R13.1 |
| Q1_GATE | R13.2, R14.1, Q1.1 (G) |
| GND | Q1.2 (S), R14.2, Q2.2 (S), R16.2 |
| SSR_IN | Q1.3 (D), CN10.1 |
| BUZZER | R15.1 |
| Q2_GATE | R15.2, R16.1, Q2.1 (G) |
| BUZ_LO | Q2.3 (D), BZ1.2, D1.A |
| +3V3 | BZ1.1, D1.K, CN10.2 |

## 4. Hinweise

- SSR (40 A) schaltet 230 V zum Kugelhahn – **nur am SSR-Ausgang** (Snubber/Varistor).
- **SSR-Verdrahtung (CN10):** Pin 2 (`+`) an **+3V3**, Pin 1 (`−`) an `Q1.3 (D)` (Low-Side-Schalter).
- **SSR-Mindestlast prüfen:** 40-A-SSR an kleinem Kugelhahn (< 50 mA) schaltet evtl. nicht sauber → ggf. kleines Relais/2-A-SSR (z. B. Omron G3MB-202P).
- Buzzer ist **passiv** → GPIO39 liefert Rechtecksignal (z. B. 2,7 kHz) über Q2.
- BZ1-Footprint **4 Pads**: 2 elektrisch (Pin 1 = +3V3, Pin 2 = BUZ_LO), 2 nur mechanische Befestigung (NC).
- D1 (1N4148W) als Freilauf über BZ1: Anode → Drain (BUZ_LO), Kathode → +3V3.
- R14/R16 (10 kΩ) halten das Gate beim ESP32-Boot sicher auf GND (Q1/Q2 gesperrt).
- AO3400A: R_DS(on) ~35 mΩ bei 3,3 V → vernachlässigbarer Spannungsabfall, volle 3,3 V an SSR/Buzzer (kein V_CE(sat) wie beim SS8050).
