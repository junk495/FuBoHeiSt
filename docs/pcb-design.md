# PCB-Design & Layout-Notizen

Praktische Merker für das Zeichnen in EasyEDA Pro – geht über das Netzlisten-Konzept (`schaltplan-konzept.md`) hinaus.
Stand: vor Schaltplan-Erstellung.

## 1. Allgemeine Layout-Prinzipien

- **230-V-Zone (SSR-Ausgang → Kugelhahn)** räumlich strikt von der Niederspannungs-Logik trennen (Kriech-/Luftstrecken).
- **Analoge Messkette** (ISEN-Shunt → MCP602 → LM393) kurz halten und **weit weg vom SSR** verlegen.
- Durchgängige **Massefläche** (Ground-Plane) auf beiden Lagen; keine langen GND-Schlaufen.
- 100-nF-Entkopplung **direkt am Pin** jedes IC.

## 2. Bauteil-spezifische Merker

### DRV8833CPWPR (HTSSOP-16, PowerPAD)
- **PowerPAD (Pin 17, Unterseite) zwingend an GND** löten, mit **4–6 Thermal-Vias** in die untere Ground-Plane.
- **Ladungspumpe (Pflicht, direkt am Chip):**
  - 10 nF zwischen **VCP (Pin 14)** und **VM (Pin 12)**.
  - 10 nF zwischen **CP1 (Pin 16)** und **CP2 (Pin 15)**.
  - 10 µF an **VINT (Pin 11)** → GND.
- **nSLEEP** an GPIO18 + **10 kΩ Pull-down** an GND.
- **Reserve-Kanal (Chip 4, Kanal B):** BIN1/BIN2 fest auf **GND**.

### LM393DR2G (SOIC-8) – Kanal 2 stilllegen
- **NICHT** als Spannungsfolger beschalten (ein Komparator würde schwingen!).
- `2IN+` (Pin 5) → **GND**, `2IN−` (Pin 6) → **3,3 V** (oder über 10 kΩ), `2OUT` (Pin 7) → **NC (offen)**.
  → IN− > IN+ ⇒ der Open-Collector-Ausgang zieht dauerhaft 0 mA.

### MCP602-I/SN (SOIC-8) – Kanal B stilllegen
- Als **Spannungsfolger** (OpAmp, kein Komparator): `IN2−` (Pin 6) ↔ `OUT2` (Pin 7), `IN2+` (Pin 5) → **GND**.

### ME6217C33M5G (SOT-23-5)
- **CE (Pin 3) → VIN (Pin 1, 5 V)** verbinden, sonst bleibt `3,3 V_MOTOR` dauerhaft ausgeschaltet.

### INA219AIDR (SOIC-8)
- Pinout: **1=IN+, 2=IN−, 3=GND, 4=VS, 5=SCL, 6=SDA, 7=A0, 8=A1** (A0/A1 → GND = Adresse 0x40).
- Externer **0,5 Ω Shunt (2× 1 Ω 0805 parallel)** zwischen IN+ und IN−.

## 3. Leiterbahnen & Routing

- **XH2.54-Motorleitungen** (AOUT1/AOUT2, BOUT1/BOUT2 → XH2.54 2-pol, Pin 1+2): **0,4–0,5 mm** Breite.
- **ISEN-Shunt:** alle 8 AISEN/BISEN → gemeinsamer Knoten → 2× 1 Ω 0805 parallel (= 0,5 Ω) → GND. **Abgriff für R1=1 kΩ + C1=100 nF vom gemeinsamen Knoten** (vor den Widerständen); Verbindung zum MCP602 kurz halten, fern vom SSR.
- **I²C (SDA/SCL):** 4,7 kΩ Pullups nahe am INA219.
- **1-Wire (GPIO40):** 4,7 kΩ Pullup.
- **Komparator-Ausgang → PCNT (GPIO38):** Pullup 10 kΩ an 3,3 V.

## 4. Versorgung & Entkopplung

- 5-V-Eingang: **Feinsicherung + TVS**.
- ME6217: 100 nF + 10 µF Bulk am Ausgang.
- 3,3 V_MOTOR → 0,5 Ω Shunt (INA219, 2× 1 Ω parallel) → VM aller DRV8833.
- DevKitC-1: 5 V am 5V-Pin (eigener Bord-LDO erzeugt das 3,3 V für das ESP32-S3).

## 5. Sicherheit (230 V)

- 230 V **nur** am SSR-Ausgang (Kugelhahn).
- Snubber/Varistor am SSR-Ausgang (induktive Last).
- Kriech-/Luftstrecken zur Logik einhalten.
