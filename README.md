# FuBoHeiSt – Fußbodenheizungs-Steuerung

> Eigene, lokale Steuerung der Fußbodenheizung einer Wohnung auf ESP32-S3-Basis.
> Steuert **8 HmIP-VDMOT-Stellmotoren** (3,3 V DC; 7 Heizkreise + 1 Reserve-Kanal) + **1 Hauptventil** (motorisierter Kugelhahn, 230 V, ein/aus).
> **Offline-first:** Der Regelkreis läuft komplett lokal – keine Cloud- oder Internet-Abhängigkeit.

> **Gesamtkonzept** (Was/Warum/Wie + KI-Integration): siehe [`docs/gesamtkonzept.md`](docs/gesamtkonzept.md).
> Dieses README ist die **technische Spec** (Hardware + Firmware + Datenmodell).

## 1. Ziel & Umfang

- **7 Heizkreise** regeln (nur Ventile, kein Eingriff in die Vorlauftemperatur).
- **1 Master-Ausgang** für den Gesamtdurchfluss (Hauptventil, 230 V, ein/aus).
- **Integrierte Regelung:** Raumtemperatur je Zone, Außentemperatur (Vorsteuerung), Bodentemperatur modellbasiert.
- **Zuverlässig & wartungsarm:** vollsolid-state Motoransteuerung, kein zentraler Ausfallpunkt.
- **KI nur in der Entwicklung** (Datenanalyse/Parameter-Tuning), nicht im laufenden Regelkreis.

### Technische Rahmenbedingungen HmIP-VDMOT
- Reiner DC-Getriebemotor, 3,3 V, Blockierstrom (Stall) ≈ 120 mA.
- 4P4C-Stecker (RJ9/RJ10-Typ) am Motor – Kabel wird gekürzt, PCB-Anschluss über XH2.54 2-pol.
- Richtung über Umpolung → H-Brücke (DRV8833).
- Endanschlag über Stall-Detection (Stromanstieg), Zwischenpositionen über Ripple-Counting.
- Getriebe selbsthemmend → kein Haltestrom nötig.

## 2. Heizkreis- & Sensorplan

**Reale Heizkreise am Verteiler** (Reihenfolge links → rechts; Quelle: [`docs/heizung-system.md`](docs/heizung-system.md)):

| Kreis | Label | Raum | Belag | Max. Bodentemp. | Raumfühler |
|---|---|---|---|---|---|
| 1 | WZ1 (85 m) | Wohnzimmer | Holz | 27 °C | WohnKue (geteilt) |
| 2 | WZ2 (85 m) | Wohnzimmer | Holz | 27 °C | WohnKue (geteilt) |
| 3 | KÜ (80 m) | Küche | Fliesen | 29 °C | WohnKue (geteilt) |
| 4 | Gang (60 m) | Vorraum | Fliesen | 29 °C | Gang |
| 5 | Bad (55 m) | Bad + WC | Fliesen | 29 °C | Bad **und** WC |
| 6 | SZ2 (100 m) | Kinderzimmer | Holz | 27 °C | KiZi |
| 7 | SZ1 (90 m) | Schlafzimmer | Holz | 27 °C | Schlafz |

- **7 Heizkreise, 7 Ventile, 6 Raumfühler** (+ 1 verkabelter Reserve-Kanal).
- **Wohnküche:** WZ1 + WZ2 + KÜ = ein großer Raum → 1 gemeinsamer Fühler (`WohnKue`), 3 unabhängige Ventile mit unterschiedlichen Boden-Grenzen (Holz 27 °C / Fliesen 29 °C).
- **Bad + WC:** Kreis „Bad" versorgt zwei getrennte Räume mit je eigenem Fühler (`Bad`, `WC`) → 1 Ventil, 2 Fühler.
- **Vorlauftemperatur-Fühler:** 1× DS18B20 (Haupt-Vorlauf) + geplant 7× DS18B20 (je Kreis) = **8 Stück** (1-Wire, GPIO40).
- **Bodentemperatur modellbasiert** (Kalibrierphase: Messung → Modell). **Kein permanenter Bodensensor** (Entscheidung); max. gemessene Vorlauftemperatur ≈ 32 °C. Temporäre Bodensensoren für die Entwicklung werden erwogen (offen).

## 3. Hardware-Architektur

### 3.1 Topologie (Variante A – festgelegt)
1 H-Brücke je Motor (**4× DRV8833** = 8 Kanäle: 7 Heizkreise + 1 voll verkabelter Reserve-Kanal für Hot-Swap), **1 geteilte Messkette** (nur 1 Motor läuft gleichzeitig).

```
                    +-------------------------------------------+
extern 5 V -------->| PCB                                       |
                    |  |- 5 V ----------> ESP32-S3-DevKitC-1    |
                    |  |                  (eigener LDO -> 3,3 V) |
                    |  `- ME6217 (LDO) -> 3,3 V_MOTOR           |
                    |                     -> [Shunt 0,5 Ω (INA219)] -> DRV8833 (VM)  |
                    +-------------------------------------------+

ESP32-S3 (GPIO)
  |- 16x IN1/IN2 --> 8 Motoren (4x DRV8833)
  |- I2C -----------> INA219 (Stall-Erkennung)
  |- PCNT ----------> LM393 (Ripple)  <--  Shunt (ISEN, Low-Side) -> MCP602
  |- GPIO ----------> SSR -> Hauptventil (230 V)
  `- GPIO (1-Wire) -> Vorlauf-/Bodenfuehler (DS18B20)
```

### 3.2 Signalaufbereitung Strommessung
```
Motorstrom:
  ├─ INA219 (High-Side, ext. Shunt 0,5 Ω) ──► I²C ──► Stall-Erkennung (DC)
  └─ ISEN-Shunt (Low-Side, 0,5 Ω) ──► Tiefpass ──► MCP602 (Gain) ──► LM393 (Data Slicer) ──► PCNT (Ripple)
```

### 3.3 Bauteileliste (Konzept)

**Hauptplatine**
| Bauteil | Funktion | Menge | Status |
|---|---|---|---|
| ESP32-S3-DevKitC-1 (S3-WROOM-1-N16R8) | Controller + WLAN/BLE 5 | 1 | vorhanden |
| DRV8833 (Dual-H-Brücke) | Motor-Treiber | 4 | bestellt |
| INA219 (ext. Shunt 0,5 Ω) | Strommessung (Stall, I²C) | 1 | vorhanden |
| Shunt 2× 1 Ω 0805 parallel = 0,5 Ω (Low-Side ISEN) | Strommessung | 2 | haben |
| MCP602 (OpAmp, SOIC-8) | Ripple-Verstärkung | 1 | vorhanden |
| LM393DR2G (Komparator) | Ripple → Digitalpulse | 1 | bestellt |
| XH2.54 2-pol | Anschluss VDMOT (Motor, 8×: 7 Heizkreise + 1 Reserve) | 8 | haben |
| SSR (3,3 V, 40 A) | Hauptventil (230 V) | 1 | vorhanden |
| ME6217 (LDO) | 5 V → 3,3 V_MOTOR | 1 | vorhanden |
| externes 5-V-Netzteil | Versorgung | 1 | kaufen (falls nicht vorhanden) |

**Sensorik**
| Bauteil | Funktion | Menge | Status |
|---|---|---|---|
| BLE-Funksensor (Modell offen) | Raumtemp. | 6 | offen |
| DS18B20-Anlegefühler | Vorlauf | 1 | kaufen |
| DS18B20-Bodenfühler | nur temporär (Entwicklung) | 0 dauerhaft | offen |

## 4. Firmware-Architektur

### 4.0 Entwicklungsumgebung (Devcontainer)

Die Entwicklung läuft in einem **VS-Code-Devcontainer** (`mcr.microsoft.com/devcontainers/base:resolute`).
Rebuilds sind dadurch reproduzierbar und die Toolchain bleibt erhalten.

| Komponente | Zweck |
|---|---|
| `pioarduino.pioarduino-ide` | PlatformIO-Fork mit ESP32/Espressif-Support |
| `jason2866.esp-decoder` | Serial-Monitor + Guru-Meditation-/Crash-Decoder |
| `ms-vscode.cpptools` | C/C++-IntelliSense-Backend (pioarduino ≥ 1.4 installiert es **nicht** mehr selbst) |

**Automatisierung:** `.devcontainer/setup.sh` läuft als `postCreateCommand` bei jedem Rebuild und ist idempotent:

1. installiert den PlatformIO-Core (pioarduino) per `pipx` nach `/usr/local/py-utils/bin` → `pio` ist im Terminal verfügbar,
2. installiert `espressif32`-Plattform, Toolchain (xtensa/riscv32) und Arduino-Framework,
3. erzeugt die cpptools-IntelliSense-Konfiguration (`.vscode/c_cpp_properties.json`).

**Persistenz:** `~/.platformio` liegt auf dem Volume `platformio-core-<projekt>` (≈ 5 GB).
Ohne dieses Volume müsste die Toolchain bei jedem Rebuild neu geladen werden (≈ 7 min).

> **Hinweis:** Die ESP32-Serial-Schnittstelle ist im Container nicht automatisch durchgereicht.
> Für Flash/Monitor muss der Port per `runArgs` (`--device=...`) durchgereicht werden.

### 4.1 Module
| Modul | Aufgabe |
|---|---|
| `MotorDriver` | DRV8833 IN1/IN2: Richtung, PWM, Brake/Coast |
| `CurrentSense` | INA219 (I²C) auslesen + Mittelwert |
| `RippleCounter` | PCNT zählt Kommutator-Pulse (Richtung kennt die Firmware) |
| `StallDetector` | Strom > Schwelle für X ms → Stop |
| `ValveController` | Zustandsmaschine je Zone |
| `PositionManager` | Kalibrierung, Pulse ↔ %, NVS |
| `ZoneScheduler` | Round-Robin: 1 Motor nach dem anderen |
| `Thermostat` | PI + Vorsteuerung + Boden-/Frost-Grenzen |
| `Comm` | MQTT + OTA |
| `Supervisor` | Timeouts, Watchdog, Fehlerverhalten |

> **Reserve-Kanal (8. Motor):** Der 8. Kanal (U5-B, GPIO41/42 → CN8) ist firmware-seitig als normaler Motor-Kanal adressierbar, wird aber nicht in den 7-Zonen-Regelkreis eingebunden. Bei Kanal-Ausfall lässt sich der betroffene Motor auf CN8 umstecken (nur im Coast/Idle) und per Konfig demselben Regelauftrag zuordnen (Hot-Swap).

### 4.2 Positionsregelung (Zustandsmaschine je Zone)
```
IDLE → MOVE → STOP → IDLE
MOVE: H-Brücke ein, Richtung setzen, PCNT starten, INA219 überwachen
STOP-Bedingungen (erste gewinnt):
  1. PCNT erreicht Ziel-Pulse → Brake → Position = Ziel
  2. Strom > Stall-Schwelle (entprellt) → Brake → Endanschlag
  3. Timeout → Brake → FEHLER (neu referenzieren)
Kalibrierung (einmalig, mit Leerhub):
  - "ganz zu" (Stall) = 0 Pulse, "ganz auf" (Stall) = P_ges
  - P_touch = Federkontakt (INA219-Stromanstieg) → Totzone davor
  - Regelbereich 0–100 % zwischen P_touch und P_open
  - Bremsweg messen und kompensieren

Auto-Homing (gegen Ripple-Zähl-Drift):
  - Soll ≤ 5 % oder ≥ 95 % → zwingend bis zum Stall, Zähler hart auf 0 / P_ges
  - nach ~50 Teilbewegungen ohne Endanschlag einmal referenzieren
```

### 4.3 Regelung (träge FBH)
- Abtastzeit ~60 s; PI mit Anti-Windup; Ausgang ratenbegrenzt.
- Vorsteuerung über Außentemperatur (antizipiert Wärmebedarf).
- `max_Bodentemperatur` je Zone (Holz 27 °C, Fliese 29 °C) → Ventil drosseln.
- Bodenmodell als zeitdiskretes RC-Glied (thermische Trägheit, τ ~45–90 min) – kein statisches f(...).
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
- Zonenliste: Name, Belag, `max_Bodentemp`, `P_ges`, `P_touch`, Totband
- Reglerparameter je Zone (PI + Vorsteuerung)
- Master-Logik (Nachlauf, Frostschutz-Schwelle)
- MQTT-Broker + OTA-Einstellungen

## 6. Offene Punkte (TODO)
- [x] Hauptventil: SSR (3,3 V, 40 A) – geklärt
- [x] BLE-Sensor-Modell festgelegt: LYWSD03MMC mit pvvx-Firmware, Advertising **BTHome v2** (Details in `docs/sensorik.md`)
- [x] Vorlauftemperatur gemessen (max. ≈ 32 °C) → **kein permanenter Bodensensor** (Entscheidung)
- [ ] Temporäre Bodensensoren für Entwicklung/Simulation: sinnvoll? (offen)
- [ ] Holz-Bodentemperatur-Grenzwert final nach Herstellerangabe prüfen
- [x] Pin-Belegung finalisiert

## 7. Nächste Schritte
1. Schaltplan-/Netzlisten-Konzept finalisieren (`docs/schaltung/` 00–08)
2. EasyEDA-Schaltplan + Trägerplatine (DRV8833, INA219, Ripple-Kette, SSR)
3. Firmware-Grundgerüst (PlatformIO + Arduino-ESP32, ESP32-S3)
4. Kalibrier-/Logging-Phase → thermisches Modell (KI-Tuning)

### Firmware-Stand (Logging)
Die Firmware (`firmware/src`) liest aktuell **6 BLE-Raumfühler** (BTHome v2) sowie
**DS18B20-Temperaturfühler (1-Wire)** aus und pusht die **Raumtemperaturen** (Feld 1–6)
sowie die **konfigurierten DS18B20-Felder** an **ThingSpeak**.

- WLAN-Zugangsdaten werden **nicht fest kodiert**, sondern beim ersten Start über ein
  **Captive-Portal** (Hotspot „FuBoHeiSt-Setup") eingegeben und dauerhaft im **NVS**
  gespeichert. Sie bleiben bei einem normalen Upload (`pio run -t upload`) erhalten;
  nur `pio run -t erase` („Erase Flash") löscht sie. Verbindung **und** Portal laufen
  **nicht-blockierend** in `loop()`, damit die serielle Ventilsteuerung auch **ohne WLAN**
  sofort bedienbar bleibt.
- Die WLAN-Verbindung **verbindet sich automatisch neu** (Reconnect mit Backoff), wenn
  sie z. B. nach einem Router-Neustart abbricht; erst nach mehreren Fehlversuchen wird
  das Portal geöffnet. Diagnose/Reset über die seriellen Befehle `w` (Status) und
  `wr` (Zugangsdaten löschen → Portal).
- Konfiguration (Fühlerliste, Intervalle) in `firmware/include/config.h`.
- **DS18B20-Sensoren (1-Wire, bis zu 16)** hängen parallel an **GPIO40** und werden über
  **OneWireNg** per eindeutiger 64-Bit-Adresse gelesen. Die Sensorliste (Name, Adresse,
  ThingSpeak-Feld) wird in **`firmware/include/ds18b20_config.h`** gepflegt; Adresse + Temperatur
  listet der serielle Befehl **`s`** auf. `field = 0` = nicht an ThingSpeak senden.
  OneWireNg statt der klassischen `OneWire`-Lib: deren ESP32-Portierung verweigert für
  GPIOs > 33 den Ausgangsmodus (Überbleibsel vom klassischen ESP32) und konnte GPIO40
  daher nicht ansteuern.
- Secrets (ThingSpeak-Key) in `firmware/include/secrets.h` (gitignored, Vorlage: `secrets.example.h`).
- **Offline-first bleibt gewahrt:** ThingSpeak ist nur Logging (Entwicklung/Auswertung).
  Fällt das Internet aus, läuft der spätere Regelkreis lokal weiter.
- Der Motor-Bringup (Stufe 1) liegt als Referenz in `firmware/test/motor_bringup/`.
- Die **nicht-blockierende Ventilsteuerung** (`ValveController`) ist in `firmware/src`
  integriert und über das serielle Interface erreichbar (`k<N>`, `v<N>`, `i`, `t`, `s`, `w`, `wr`, `h`).
  Die Ventilfahrt hat **Vorrang**: während einer Fahrt wird kein neuer BLE-Zyklus gestartet
  (ein bereits laufender, nicht-blockierender Scan läuft im Hintergrund weiter).
  Ein benutzerfreundlicheres Interface (Web/MQTT) folgt später.

### Geplante Projektstruktur
```
FuBoHeiSt/
├── README.md                  (Spec)
├── AGENTS.md                  (Arbeitsweise für Agenten)
├── .clinerules                (Cline-Regeln)
├── .devcontainer/             (Devcontainer: devcontainer.json + setup.sh)
├── platformio.ini            (PIO-Projekt im Root)
├── docs/gesamtkonzept.md      (Gesamtkonzept: Was/Warum/Wie + KI)
├── docs/pcb-design.md         (PCB-Layout-Notizen)
├── docs/pcb-placement.md      (Bauteil-Platzierung & Gruppierung)
├── docs/esp32_devkit.md       (DevKitC-1-Referenz: Pinout + Hinweise)
├── docs/bom.md                (Stückliste: vorhanden vs. kaufen)
├── docs/schaltung/            (Schaltungs-Gruppen 00–08, EasyEDA-Netzlisten)
├── docs/sensorik.md           (Raumfühler-Recherche: BTHome v2, Modell offen)
├── hardware/                  (EasyEDA, später)
└── firmware/                  (Firmware-Quellcode: src/ + include/)
```

