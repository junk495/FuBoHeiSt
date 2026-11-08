# PCB-Platzierung – Gruppierung & Anordnung

> Praktische Platzierungshilfe für EasyEDA: **welche Bauteile zusammen gehören** und
> **wo sie relativ zueinander hin müssen**.
> Ergänzt die Layout-Regeln aus [`pcb-design.md`](pcb-design.md) (dort: 230-V-Trennung,
> Massefläche, Entkopplung). RefDes/Netze: [`schaltung/00-referenz.md`](schaltung/00-referenz.md).

## 1. Funktionsgruppen (alle Bauteile zugeordnet)

| Gruppe | RefDes | Funktion |
|---|---|---|
| **G1 – Power** | CN9, F1, D2, U9, C18, C19 | 5-V-Eingang → PTC/TVS → ME6217 → 3,3 V_MOTOR |
| **G2 – Strommessung** | U6, R17, R18, R10, R11, C15 | INA219 (I²C) am High-Side-Shunt 0,5 Ω |
| **G3 – Motor-Treiber** | U2–U5, C3–C6, C7–C10, C11–C14, C20, R9, R21 | 4× DRV8833 (8 Kanäle) |
| **G4 – Motor-Verbinder** | CN1–CN8 | XH2.54 2-pol (7 Zonen + 1 Reserve) |
| **G5 – Ripple-Messkette** | R19, R20, R1, C1, U7, R2, R3, C16, U8, R4, R5, C2, R6, R7, R8, C17 | ISEN-Shunt → Tiefpass → MCP602 → LM393 |
| **G6 – Ausgänge** | Q1, R13, R14, CN10, Q2, R15, R16, BZ1, D1 | SSR-Treiber (Hauptventil) + Buzzer |
| **G7 – 1-Wire** | CN11, CN12, R12 | DS18B20 (Vorlauf + optional Bodenfühler) |
| **G8 – Debug/Test** | CN-UART, TP1–TP14 | UART-Debug-Header + Messpads |
| **U1 – Controller** | U1 | ESP32-S3-DevKitC-1 (Steckmodul) |

> **Vollständigkeit:** Alle RefDes aus BOM + Schaltungs-Gruppen sind hier einer Gruppe
> zugeordnet. SSR (230 V) und DS18B20-Fühler sind **extern**, nicht auf der PCB.

## 2. Anordnungs-Skizze (relative Lage)

```
          Platinenrand – Verbinderseite (Stecker nach außen)
┌─────────────────────────────────────────────────────────────────────┐
│ CN9(5V)  F1  D2  U9  C18 C19 │ CN1 CN2 CN3 CN4 CN5 CN6 CN7 CN8 │
│      [G1 Power]              │         [G4 Motor-Verbinder]    │
│──────────────────────────────────────────────────────────────────│
│  [G2: U6 INA219 + R17/R18 + R10/R11 + C15]                       │
│                              ┌─────────────────────────────┐    │
│                              │  U1  ESP32-S3-DevKitC-1     │    │
│                              │  (Steckmodul, 2×22-Leiste)  │    │
│                              └─────────────────────────────┘    │
│                              │  [G3: U2 U3 U4 U5 + C3–C14,   │    │
│                              │        C20, R9, R21]           │    │
│──────────────────────────────────────────────────────────────────│
│  [G5 Ripple: R19/R20, R1/C1, U7, U8 + R2–R8, C2, C16, C17]      │
│──────────────────────────────────────────────────────────────────│
│ CN11 R12 │ Q1 R13/R14 CN10 │ Q2 R15/R16 BZ1 D1 │ CN-UART (Debug) │
│ [G7]     │ [G6 SSR]        │ [G6 Buzzer]        │ [G8]         │
└─────────────────────────────────────────────────────────────────┘
        ▲ 230-V-Zone: CN10 → SSR → Kugelhahn, Leitung strikt wegführen
```

## 3. Positionierung je Gruppe (was gehört nahe wem)

### G1 – Power (Platinen-Ecke, Eingang)
- **CN9 (5V) → F1 (PTC) → D2 (TVS)** direkt nacheinander, kurz und breit (≈0,5 mm).
- **U9 (ME6217)** nahe F1; **C18 (100 nF) direkt an U9-Pin 1**, **C19 (10 µF) direkt an U9-Pin 5**.
- G1 an **eine** Ecke, damit die 5-V-Leitung nicht durch die Analog-/Logikzone läuft.

### G2 – Strommessung (zwischen G1 und G3)
- **U6 (INA219)** zwischen **U9-Ausgang (+3V3_MOTOR)** und **VM-Knoten**.
- **R17/R18 (0,5 Ω)** unmittelbar bei U6 – Pfad: `+3V3_MOTOR → R17∥R18 → U6 IN+ / IN− → VM`.
- **R10/R11 (4,7 kΩ)** nahe U6 (I²C-Pullups an Pin 3/4), **C15 (100 nF)** an U6-Pin 5 (VS).

### G3 – Motor-Treiber (zentral, unter/neben U1)
- **U2–U5 (DRV8833)** in einer Reihe, jeder nahe seiner **zwei** Ausgangs-Stecker (CN1–CN8).
- Je Chip **direkt am Pin**: **C3–C6** (10 µF an VINT-Pin 14), **C7–C10** (10 nF VCP→VM), **C11–C14** (100 nF an VM-Pin 12).
- **C20 (10 µF VM-Bulk)** zentral zwischen den vier VM-Pins.
- **R9 (nSLEEP-Pulldown)** nahe U2 (GPIO18-Leitung), **R21 (nFAULT-Pullup)** nahe U2 (GPIO21).
- **PowerPAD (Pin 17) → GND** mit 4–6 Thermal-Vias (siehe `pcb-design.md`).

### G4 – Motor-Verbinder (Platinenrand)
- **CN1–CN8** am Rand, je 2-pol XH2.54, **Pin 1+2** (Polarität egal – H-Brücke polt um).
- Leiterbahn **0,4–0,5 mm**; CN nahe dem zugehörigen DRV8833-Kanal halten.

### G5 – Ripple-Messkette (analog, fern vom SSR)
- **R19/R20 (ISEN-Shunt 0,5 Ω)** am gemeinsamen ISEN-Knoten **nahe G3** (alle 8 AISEN/BISEN laufen dort zusammen).
- **R1/C1 (Tiefpass) → U7 (MCP602) → U8 (LM393)** kompakt beieinander – **kurze Wege**, fern vom SSR und der 230-V-Zone.
- **R2/R3** (Gain) an U7, **R4–R8 + C2** (Data Slicer) an U8; **C16/C17** (100 nF) direkt an U7-VDD/U8-VCC.
- Komparator-Ausgang → **R8 (Pullup)** → **RIPPLE (GPIO38)**.

### G6 – Ausgänge (unten, getrennt von Analog)
- **Q1 + R13/R14** nahe **CN10** (SSR); **Q2 + R15/R16 + BZ1 + D1** als Buzzer-Block.
- **CN10 (SSR-Steuerung)** ist der **einzige** Punkt mit 230-V-Bezug → am Rand, Leitung **weg** von der Logik führen.

### G7 – 1-Wire (Rand, niederspannungsseitig)
- **CN11 (3-pol, Vorlauf)** + **CN12 (3-pol, optional Bodenfühler)** + **R12 (4,7 kΩ)** zusammen; kurzer Weg zu GPIO40.
- Pin-Belegung CN11/CN12: 1 = GND, 2 = +3V3, 3 = Daten (siehe `06-1-wire.md`).

### G8 – Debug/Test (Rand, gut zugänglich)
- **CN-UART (4-pol UART)** am Rand; Messpads **TP1–TP14** nahe am jeweiligen Bauteil platzieren
  (Details + RefDes: [`schaltung/08-testpunkte.md`](schaltung/08-testpunkte.md)).

### U1 – Controller (zentral)
- **U1 (DevKitC-1)** mittig über die **2× Buchsenleiste 1×22**; alle GPIO-Leitungen sternförmig zu den Gruppen.
- 5 V am 5V-Pin; 3V3-Pin speist die Logik-Rail (Pullups, INA219-VS).

## 4. Kritische Regeln (Reihenfolge der Wichtigkeit)

1. **230-V-Zone (CN10 → SSR) strikt getrennt** – Kriech-/Luftstrecken zur Logik.
2. **Analoge Messkette (G5) kurz halten**, fern vom SSR und der 5-V-Leistung.
3. **Entkopplung direkt am Pin** jedes IC (100 nF), Bulk (10 µF) gruppenweise.
4. **Durchgängige Ground-Plane** auf beiden Lagen; PowerPADs der DRV8833 mit Thermal-Vias.
