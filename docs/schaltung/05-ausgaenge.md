# 05 – Ausgänge (SSR + Buzzer)

> Zweck: Hauptventil (SSR, 230 V) und Fehler-Signalton (Buzzer), je über SS8050.
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
GPIO47 ─► R13 (1k) ─► Q1 (SS8050) ─► CN9 (SSR-Eingang)
                 R14 (10k) → GND

GPIO39 ─► R15 (1k) ─► Q2 (SS8050) ─► BZ1 (Buzzer)  [D1 Freilauf]
                 R16 (10k) → GND
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| Q1 | SS8050 (NPN, 1,5 A) | SOT-23 | C7420370 | SSR-Treiber |
| Q2 | SS8050 (NPN, 1,5 A) | SOT-23 | C7420370 | Buzzer-Treiber |
| R13, R15 | 1 kΩ | 0805 | — | Basis-Vorwiderstand |
| R14, R16 | 10 kΩ | 0805 | — | Base-Emitter-Pulldown |
| CN9 | XH2.54 2-pol | THT 2,54 mm | — | SSR-Steuerung |
| BZ1 | MLT-8540H (2,5–4,5 V) | SMD 8,5×8,5 | C95298 | Fehler-Signalton |
| D1 | 1N4148W | SOD-123 | C7420318 | Buzzer-Freilauf |
| SSR | 3,3 V, 40 A (extern) | — | — | Hauptventil (230 V) |

## 3. Netzliste

| Netz | Pins |
|---|---|
| SSR_CTRL | R13.1 |
| Q1_BASE | R13.2, R14.1, Q1.B |
| Q1_EMITTER | Q1.E, R14.2, GND |
| SSR_IN | Q1.C, CN9.1 |
| BUZZER | R15.1 |
| Q2_BASE | R15.2, R16.1, Q2.B |
| Q2_EMITTER | Q2.E, R16.2, GND |
| BUZ_LO | Q2.C, BZ1.2, D1.A |
| +3V3 | BZ1.1, D1.K |

## 4. Hinweise

- SSR (40 A) schaltet 230 V zum Kugelhahn – **nur am SSR-Ausgang** (Snubber/Varistor).
- **SSR-Mindestlast prüfen:** 40-A-SSR an kleinem Kugelhahn (< 50 mA) schaltet evtl. nicht sauber → ggf. kleines Relais/2-A-SSR (z. B. Omron G3MB-202P).
- Buzzer ist **passiv** → GPIO39 liefert Rechtecksignal (z. B. 2,7 kHz) über Q2.
- D1 (1N4148W) als Freilauf über BZ1: Anode → Kollektor (BUZ_LO), Kathode → +3V3.
- R14/R16 (10 kΩ) sorgen dafür, dass Q1/Q2 beim ESP32-Boot sicher gesperrt sind.
