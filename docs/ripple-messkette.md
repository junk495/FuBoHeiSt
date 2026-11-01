# Ripple-Messkette – verdrahtungsfertige Beschaltung

Komplette, zeichenfertige Beschaltung der Strom-/Ripple-Messung mit allen R/C-Werten,
Pin-Nummern und Spannungspegeln. Zwei getrennte Shunt-Pfade.

## 1. Überblick

```
                       ┌───────────────────────────────────────────────┐
3,3 V_MOTOR ──►[0,5 Ω Shunt]──► VM (4× DRV8833)                        │
     │            │  ▲                                                 │
     │            └──┼── INA219 (IN+ Pin1 / IN− Pin2) ──► I²C ──► Stall │
     │               │                                                  │
     └── ME6217 (LDO, 5 V → 3,3 V)                                      │
                                                                        │
ISEN (alle 8 AISEN/BISEN) ──►[0,5 Ω = 2× 1 Ω parallel]──► GND            │
     │                                                                  │
     └─► R1(1k)+C1(100n) ─► MCP602 (Gain 23) ─► LM393 (Data Slicer) ─► PCNT (GPIO38)
                                                                        │
                       └───────────────────────────────────────────────┘
```

## 2. Pfad A – INA219 (Stall-/DC-Strom, High-Side)

- **Shunt:** 0,5 Ω (2× 1 Ω 0805 parallel) zwischen **IN+ (Pin 1)** und **IN− (Pin 2)**.
- **Pfad:** `3,3 V_MOTOR → IN+ → [0,5 Ω] → IN− → VM (alle DRV8833)`.
- **INA219AIDR (SOIC-8) Pinout:**
  - Pin 1 = IN+, Pin 2 = IN−, Pin 3 = GND, Pin 4 = VS (3,3 V),
  - Pin 5 = SCL (GPIO2), Pin 6 = SDA (GPIO1),
  - Pin 7 = A0 → GND, Pin 8 = A1 → GND (→ Adresse **0x40**).
- **Pullups:** 4,7 kΩ an SDA und SCL → 3,3 V.
- **Signal:** 120 mA × 0,5 Ω = **60 mV** (Anlauf-Peak ~180–250 mA → 90–125 mV); INA219-PGA **÷4 (±160 mV)** oder **÷8 (±320 mV)** → Software-Schwelle für Stall.

## 3. Pfad B – ISEN-Shunt + Ripple-Kette (Low-Side)

### 3.1 ISEN-Shunt
- Alle 8 AISEN/BISEN-Pins → **gemeinsamer Knoten** → **0,5 Ω (2× 1 Ω 0805 parallel)** → GND.
- Am ISEN fließt der Strom immer positiv Richtung GND (H-Brücke richtet intern gleich) → massebezogenes DC+Ripple-Signal, **kein Bias nötig**.
- Spannungen: **25 mV** @ 50 mA, **60 mV** @ 120 mA Stall; Ripple ~±5 mV.

### 3.2 Tiefpass (R1/C1)
- **R1 = 1 kΩ** (vom Shunt-Knoten) + **C1 = 100 nF** → GND, **fc ≈ 1,6 kHz**.
- Filtert Bürstenfeuer, lässt Kommutator-Ripple (200–1000 Hz) durch.

### 3.3 Stufe 1 – MCP602 (nicht-invertierend, Gain 23)
- **Versorgung:** VDD = **5 V** (Pin 8), VSS = GND (Pin 4), 100 nF am Pin.
- **IN1+ (Pin 3):** über R1/C1 vom Shunt.
- **IN1− (Pin 2):** Teiler **R2 = 1 kΩ → GND** und **R3 = 22 kΩ** (Feedback von OUT1, Pin 1).
- **Gain = 1 + R3/R2 = 23.**
- **OUT1 (Pin 1):** 575 mV @ 50 mA, **1,38 V** @ 120 mA Stall (kein Übersteuern).
- **Kanal B (ungenutzt) als Spannungsfolger:** IN2− (Pin 6) ↔ OUT2 (Pin 7), IN2+ (Pin 5) → GND.

### 3.4 Stufe 2 – LM393 als Data Slicer (selbstnachführend)
- **Versorgung:** VCC = **5 V** (Pin 8), GND (Pin 4).
- **IN+ (Pin 3):** schnelles Signal über **R4 = 10 kΩ** von MCP602-OUT1.
- **IN− (Pin 2):** gleitender Mittelwert über **R5 = 1 kΩ** von OUT1 + **C2 = 10 µF** → GND (τ ≈ 10 ms, fc ≈ 16 Hz — schnell genug, um beim Motoranlauf keine Pulse zu verschlucken); zusätzlich **Rbias = 22 kΩ** von 3,3 V → hebt den Ruhepegel auf ~143 mV (sicher über der +69-mV-Hysterese → stabil im Stillstand).
- **OUT (Pin 1):** Open-Collector → **Pullup 10 kΩ an 3,3 V**; **Hysterese Rhyst = 470 kΩ** von OUT auf IN+.
- **Ausgang → PCNT (GPIO38).**
- **Kanal 2 (ungenutzt):** 2IN+ (Pin 5) → GND, 2IN− (Pin 6) → 3,3 V, 2OUT (Pin 7) → **NC** (offen).

## 4. Spannungspegel (Startwerte)

| Punkt | @ 50 mA Lauf | @ 120 mA Stall (blockiert) |
|---|---|---|
| ISEN-Shunt (0,5 Ω) | 25 mV DC, ±5 mV Ripple | 60 mV DC, **0 mV Ripple** (Motor steht) |
| MCP602 OUT1 (Gain 23) | 575 mV DC, ±115 mV Ripple (230 mVpp) | 1,38 V DC, **0 mV Ripple** |
| LM393 IN− (Mittel + Offset) | ~OUT1 + Offset | ~OUT1 + Offset |
| LM393 OUT → PCNT | Rechteck bei Ripple-Frequenz (0 / 3,3 V) | **konstant, keine Pulse** |

## 5. Stückliste (nur diese Kette)

| Bauteil | Wert | Menge | Anmerkung |
|---|---|---|---|
| Shunt High-Side | 1 Ω 0805 (1 %), 2× parallel | 2 | INA219 (= 0,5 Ω) |
| Shunt Low-Side | 1 Ω 0805 (1 %), 2× parallel | 2 | ISEN (= 0,5 Ω) |
| R1 | 1 kΩ | 1 | Tiefpass |
| C1 | 100 nF | 1 | Tiefpass |
| R2 | 1 kΩ | 1 | Gain-Teiler |
| R3 | 22 kΩ | 1 | Gain-Feedback |
| R4 | 10 kΩ | 1 | Slicer schnell |
| R5 | 1 kΩ | 1 | Slicer Mittelwert |
| C2 | 10 µF | 1 | Slicer Mittelwert |
| Rbias | 22 kΩ | 1 | Slicer Offset |
| Rhyst | 470 kΩ | 1 | Hysterese |
| Rpullup | 10 kΩ | 1 | LM393 Open-Collector |
| C_abblock | 100 nF | 3 | Entkopplung INA219 (VS), MCP602 (VDD), LM393 (VCC) |
| R_pullup | 4,7 kΩ | 2 | I²C-Pullups (SDA/SCL) |
| MCP602-I/SN | Dual-OpAmp | 1 | Stufe 1 |
| LM393DR2G | Dual-Komparator | 1 | Stufe 2 |
| INA219AIDR | Strom-Monitor | 1 | Pfad A |

## 6. Hinweise

- Alle Werte sind **Startwerte** (Stall-Strom/Ripple noch nicht gemessen) → auf der ersten Platine nachjustieren (Gain über R3, Slicer-Zeit über R5/C2).
- **Gesamt-Spannungsabfall @ 120 mA Stall:** 60 mV (INA219-Shunt) + ~43 mV (DRV8833 R_DS(on)) + 60 mV (ISEN-Shunt) ≈ **163 mV** → **3,14 V** am Motor (für den 3,3-V-VDMOT unkritisch).
- MCP602 & LM393 an **5 V**; Pullup an **3,3 V** (PCNT-sicher für GPIO38).
- Analog-Leitungen (Shunt → MCP602 → LM393) kurz und fern vom SSR (230 V) führen.
