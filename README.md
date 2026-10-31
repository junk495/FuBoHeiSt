# FuBoHeiSt – Fußbodenheizungs-Steuerung

> Eigene, lokale Steuerung der Fußbodenheizung einer Wohnung auf ESP32-Basis.
> Steuert **7 HmIP-VDMOT-Stellmotoren** (3,3 V DC) + **1 Hauptventil** (motorisierter Kugelhahn, 230 V, ein/aus).
> **Offline-first:** Der Regelkreis läuft komplett lokal – keine Cloud- oder Internet-Abhängigkeit.

## 1. Ziel & Umfang

- **7 Heizkreise** regeln (nur Ventile, kein Eingriff in die Vorlauftemperatur).
- **1 Master-Ausgang** für den Gesamtdurchfluss (Hauptventil, 230 V, ein/aus).
- **Integrierte Regelung:** Raumtemperatur je Zone, Außentemperatur (Vorsteuerung), Bodentemperatur modellbasiert.
- **Zuverlässig & wartungsarm:** vollsolid-state Motoransteuerung, kein zentraler Ausfallpunkt.
- **KI nur in der Entwicklung** (Datenanalyse/Parameter-Tuning), nicht im laufenden Regelkreis.

### Technische Rahmenbedingungen HmIP-VDMOT
- Reiner DC-Getriebemotor, 3,3 V, Blockierstrom (Stall) ≈ 120 mA.
- RJ12/6P6C-Stecker, nur die 2 mittleren Adern belegt.
- Richtung über Umpolung → H-Brücke (DRV8833).
- Endanschlag über Stall-Detection (Stromanstieg), Zwischenpositionen über Ripple-Counting.
- Getriebe selbsthemmend → kein Haltestrom nötig.

## 2. Zonen- & Sensorplan

| Zone | Raum | Belag | Max. Bodentemp. | Raumfühler |
|---|---|---|---|---|
| 1 | Bad | Fliesen | 29 °C (Bad bis ~33) | 1 |
| 2 | WC | Fliesen | 29 °C | 1 |
| 3 | Vorraum | Fliesen | 29 °C | 1 |
| 4 | Küche | Fliesen | 29 °C | ⤵ geteilt mit WZ |
| 5 | Wohnzimmer | Holz | 27 °C | ⤴ geteilt mit Küche |
| 6 | Schlafzimmer | Holz | 27 °C | 1 |
| 7 | Kinderzimmer | Holz | 27 °C | 1 |

- **7 Zonen, 7 Ventile, 6 Raumfühler** (Küche + Wohnzimmer = ein offener Raum mit 2 getrennten Kreisen → 1 gemeinsamer Fühler, 2 unabhängige Ventile mit unterschiedlichen Boden-Grenzen).
- **1 Vorlauftemperatur-Fühler** (DS18B20-Anlegefühler, 1-Wire).
- **Bodentemperatur modellbasiert** (Kalibrierphase: Messung → Modell); **optional 1 Bodensensor** nur, falls Vorlauf > ~32 °C.

## 3. Hardware-Architektur

### 3.1 Topologie (Variante A – festgelegt)
1 H-Brücke je Motor (**4× DRV8833** = 8 Kanäle, 1 Reserve), **1 geteilte Messkette** (nur 1 Motor läuft gleichzeitig).

```
                    ┌─────────────────────────────────────────────────┐
230 V AC ──►[AC-DC 5V]──►[Buck]──► 3.3V_LOGIC (ESP32, Sensoren)       │
                HLK-PM05           └──► 3.3V_MOTOR ─►[Shunt]─►H-Brücken │
                                     │                                 │
                    ESP32            │                                 │
                     ├─ GPIO ×14 ────► 7× (IN1/IN2) DRV8833            │
                     ├─ ADC ────────► INA180 (Stall)                   │
                     ├─ PCNT ───────► Komparator (Ripple)              │
                     ├─ GPIO ───────► Relais → Hauptventil (230 V)      │
                     └─ GPIO (1-Wire)► Vorlauf-/Bodenfühler             │
                    └─────────────────────────────────────────────────┘
```

### 3.2 Signalaufbereitung Strommessung
```
Motorstrom → Shunt → Spannung ∝ I
                       ├─ Tiefpass ──► ADC ──► Stall-Erkennung
                       └─ Hochpass/AC ──► OpAmp ──► Komparator ──► PCNT (Ripple)
```

### 3.3 Bauteileliste (Konzept)

**Hauptplatine**
| Bauteil | Funktion | Menge |
|---|---|---|
| ESP32-WROOM-32E | Controller + WLAN/BLE | 1 |
| DRV8833 (Dual-H-Brücke) | Motor-Treiber | 4 |
| INA180 (A2/A4) | High-Side-Strommessung | 1 |
| Shunt (0,1–0,5 Ω, ≥0,25 W) | Strommessung | 1 |
| MCP6002 (OpAmp) | Ripple-Verstärkung | 1 |
| Komparator (TLV3201/MAX9141) | Ripple → Digitalpulse | 1 |
| RJ12/6P6C-Buchse | Anschluss VDMOT | 7 |
| Relais 230 V + Optokoppler + Snubber | Hauptventil | 1 |
| HLK-PM05 (5 V/5 W, isoliert) | 230 V → 5 V | 1 |
| Buck (AP63356/MP1584) | 5 V → 3,3 V | 1 |
| Feinsicherung + TVS + Bulk-Elkos | Schutz/Stabilität | div. |

**Sensorik (fertig gekauft)**
| Bauteil | Funktion | Menge |
|---|---|---|
| BLE-Funksensor (Xiaomi LYWSD03MMC o. ä.) | Raumtemp. | 6 |
| DS18B20-Anlegefühler | Vorlauf | 1 |
| DS18B20-Bodenfühler | optional | 0–1 |

## 4. Firmware-Architektur

### 4.1 Module
| Modul | Aufgabe |
|---|---|
| `MotorDriver` | DRV8833 IN1/IN2: Richtung, PWM, Brake/Coast |
| `CurrentSense` | ADC-Messung des Shunts (Mittelwert) |
| `RippleCounter` | PCNT zählt Kommutator-Pulse (Richtung kennt die Firmware) |
| `StallDetector` | Strom > Schwelle für X ms → Stop |
| `ValveController` | Zustandsmaschine je Zone |
| `PositionManager` | Kalibrierung, Pulse ↔ %, NVS |
| `ZoneScheduler` | Round-Robin: 1 Motor nach dem anderen |
| `Thermostat` | PI + Vorsteuerung + Boden-/Frost-Grenzen |
| `Comm` | MQTT + OTA |
| `Supervisor` | Timeouts, Watchdog, Fehlerverhalten |

### 4.2 Positionsregelung (Zustandsmaschine je Zone)
```
IDLE → MOVE → STOP → IDLE
MOVE: H-Brücke ein, Richtung setzen, PCNT starten, ADC überwachen
STOP-Bedingungen (erste gewinnt):
  1. PCNT erreicht Ziel-Pulse → Brake → Position = Ziel
  2. Strom > Stall-Schwelle (entprellt) → Brake → Endanschlag
  3. Timeout → Brake → FEHLER (neu referenzieren)
Kalibrierung: "ganz zu" (Stall) = 0 Pulse, "ganz auf" (Stall) = P_ges,
  Position[%] = P / P_ges × 100; Bremsweg wird gemessen und kompensiert.
```

### 4.3 Regelung (träge FBH)
- Abtastzeit ~60 s; PI mit Anti-Windup; Ausgang ratenbegrenzt.
- Vorsteuerung über Außentemperatur (antizipiert Wärmebedarf).
- `max_Bodentemperatur` je Zone (Holz 27 °C, Fliese 29 °C) → Ventil drosseln.
- Frostschutz mit Vorrang.
- Master-Logik: Hauptventil EIN wenn ≥1 Zone offen; AUS bei keinem Bedarf (mit Nachlauf).

## 5. Kommunikation & Datenmodell

### 5.1 MQTT-Topics (lokal, z. B. Mosquitto)
```
fbh/zone/<id>/target_temp      ← Soll-Raumtemp. (HA/User)
fbh/zone/<id>/current_temp     → Ist-Raumtemp.
fbh/zone/<id>/valve_target     → Ziel Ventil-% (vom Regler)
fbh/zone/<id>/valve_position   → Ist Ventil-%
fbh/zone/<id>/floor_temp       → (optional)
fbh/zone/<id>/status           → idle/moving/calibrating/error
fbh/master/flow                → ein/aus
fbh/supply/temp                → Vorlauftemperatur °C
fbh/config/...                 → Parameter (publish)
```

### 5.2 Persistente Konfig (NVS)
- Zonenliste: Name, Belag, `max_Bodentemp`, `P_ges`, Totband
- Reglerparameter je Zone (PI + Vorsteuerung)
- Master-Logik (Nachlauf, Frostschutz-Schwelle)
- MQTT-Broker + OTA-Einstellungen

## 6. Offene Punkte (TODO)
- [ ] Hauptventil-Verdrahtung: 2-adrig (→ 1 Relais) oder 3-adrig (→ 2 Relais)?
- [ ] BLE-Sensor-Modell festlegen (Xiaomi LYWSD03MMC?)
- [ ] Vorlauftemperatur messen → entscheidet, ob ein Bodensensor nötig ist
- [ ] Holz-Bodentemperatur-Grenzwert final nach Herstellerangabe prüfen
- [ ] Finale Pin-Belegung (im Schaltplan)

## 7. Nächste Schritte
1. EasyEDA-Schaltplan (Treiber + Messkette + Relais + Versorgung)
2. Firmware-Grundgerüst (PlatformIO + Arduino-ESP32)
3. Kalibrier-/Logging-Phase → thermisches Modell (KI-Tuning)

### Geplante Projektstruktur
```
FuBoHeiSt/
├── README.md        (diese Spec)
├── hardware/        (EasyEDA-Schaltplan/-PCB, später)
├── firmware/        (PlatformIO-Projekt, später)
└── docs/            (weitere Doku, später)
```

