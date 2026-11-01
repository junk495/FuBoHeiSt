# Gesamtkonzept FuBoHeiSt

Das große Bild: **was** wir regeln, **warum**, **wie** – und wie **KI** dabei hilft.
Die elektronische Umsetzung (Schaltplan, Pinbelegung, BOM, Firmware-Module) steht in `README.md` und `docs/schaltplan-konzept.md`; dieses Dokument beschreibt das Konzept darüber.

## 1. Ausgangslage & Motivation (Warum)

**Situation:**
- Wohnung mit **Fußbodenheizung**, **7 Heizkreise**, je Kreis ein **HmIP-VDMOT-Stellmotor**.
- Der HmIP-VDMOT ist ein „dummer" DC-Getriebemotor (3,3 V, ~120 mA Blockierstrom) – **keine Intelligenz, kein Bus, kein Encoder** im Gehäuse.
- Aktuell gibt es **nur 1 Raumfühler (Wohnzimmer) für alle Räume** → die Regelung funktioniert schlecht (Räume werden ungleichmäßig warm/kalt).
- **1 Hauptventil** (motorisierter Kugelhahn, 230 V, ein/aus) schaltet den Gesamtdurchfluss.

**Ziel:**
- Jeder Raum wird **individuell** auf seine Wunschtemperatur geregelt.
- Effizient, zuverlässig und **ohne Cloud-/Herstellerzwang**.

**Warum ein eigenes System statt Homematic IP / Cloud:**
- **Offline-first:** Die Heizung darf nicht von einer funktionierenden Internetverbindung abhängen.
- **Kein Herstellerzwang:** volle Kontrolle über Hardware und Firmware.
- **Energieeffizienz:** das Getriebe ist selbsthemmend → kein Haltestrom; es fährt immer nur 1 Motor gleichzeitig.
- **Bessere Regelung:** statt eines einzigen Fühlers bekommt jede Zone ihre eigene Messung.

## 2. Was wir steuern (Umfang)

| Ebene | Aktor | Bereich |
|---|---|---|
| Zonenventile | 7× HmIP-VDMOT (3,3 V DC) | 0–100 % Öffnung (hydraulischer Abgleich) |
| Gesamtdurchfluss | 1× Hauptventil (Kugelhahn, 230 V, ein/aus) | ein/aus (Sommer, Frostschutz, kein Bedarf) |
| Vorlauftemperatur | — | **nicht** unser Eingriff (setzt das Gebäude fest) |

**Regelgrößen (Eingänge):**
- Raumtemperatur je Zone (Soll/Ist).
- Außentemperatur (Vorsteuerung).
- Bodentemperatur (modellbasiert, optional 1 Sensor).

**Zonen (7 Kreise):**

| Zone | Raum | Belag | Max. Bodentemp. |
|---|---|---|---|
| 1 | Bad | Fliesen | 29 °C |
| 2 | WC | Fliesen | 29 °C |
| 3 | Vorraum | Fliesen | 29 °C |
| 4 | Küche | Fliesen | 29 °C |
| 5 | Wohnzimmer | Holz | 27 °C |
| 6 | Schlafzimmer | Holz | 27 °C |
| 7 | Kinderzimmer | Holz | 27 °C |

Sonderfall **Küche + Wohnzimmer**: ein offener Raum, aber zwei getrennte Kreise (1× Fliese, 1× Holz) → **1 gemeinsamer Raumfühler**, **2 unabhängige Ventile** mit unterschiedlichen Boden-Grenzen.

## 3. Wie wir regeln (Regelstrategie & Physik)

**Das zentrale Problem:** Fußbodenheizung ist **sehr träge** – Zeitkonstanten im Stundenbereich, große Totzeit. Ein naives PID würde deshalb schwingen.

**Deshalb:**
- **Langsame Regelung:** Abtastzeit ~60 s, Ausgang **ratenbegrenzt**, PI mit **Anti-Windup**.
- **Vorsteuerung über Außentemperatur** (Feedforward/Heizkurve): antizipiert den Wärmebedarf, statt dass der Regler die Trägheit allein ausgleichen muss.
- **Bodentemperatur-Begrenzung** je Zone: Holz ~27 °C, Fliesen ~29 °C → Ventil drosseln (Schutz + Komfort).
- **Frostschutz** mit Vorrang.
- **Master-Logik:** Hauptventil EIN, sobald ≥1 Zone Wärmebedarf hat; AUS bei keinem Bedarf (mit Nachlauf).

**Ventil-Positionierung (wie der Motor geregelt wird):**
- Endanschläge über **Stall-Erkennung** (Stromanstieg), Zwischenpositionen über **Ripple-Counting** (Kommutator-Pulse zählen).
- Kalibrierung: „ganz zu" = 0 Pulse, „ganz auf" = Gesamtpulse → Position in %.

## 4. Bodentemperatur: Modell statt Dauermessung

- Für die **Regelung/Komfort** reicht ein **Modell**: ein paar Tage messen (Boden + Raum + Vorlauf), daraus `Bodentemp = f(Ventil%, Raumtemp, Vorlauf)` fitten.
- Für den **Holz-Schutz** (27 °C) ist ein Modell riskant – ein dauerhaft beschädigter Parkettboden ist teuer. Deshalb:
  - **Vorlauftemperatur messen** (1 DS18B20-Anlegefühler am Vorlaufrohr).
  - Physikalischer Fakt: **die Bodentemperatur kann nie über die Vorlauftemperatur steigen.**
  - Liegt der Vorlauf dauerhaft ≤ ~32 °C → Holz ist sicher, das Modell genügt.
  - Liegt er höher → **1 Bodensensor** im kritischsten Holzraum als harte Grenze.

- **Wichtig fürs Modell:** Ein statisches `Bodentemp = f(...)` reicht nicht – der Boden folgt dem Ventil nur verzögert (thermische Trägheit, τ ≈ 45–90 min). Das Bodenmodell muss ein **zeitdiskretes RC-Glied (exponentiell geglätteter Zustand)** sein, sonst drosselt es das Ventil sofort nach dem Öffnen.

## 5. KI-Integration (nur Entwicklung, nicht im Regelkreis)

**Grundsatz:** KI im laufenden Regelkreis würde das Offline-first-Ziel verletzen. KI ist deshalb ein **Entwicklungs-/Tuning-Werkzeug**, kein Betriebs-Bestandteil.

1. **Logging-Phase:** Der ESP32 zeichnet über Tage/Wochen Sensordaten + Ventilstellungen auf (via MQTT auf einen PC oder SD-Karte).
2. **KI-Analyse:** Ein LLM/Modell auf dem PC identifiziert die thermische Strecke (Zeitkonstanten, Verstärkung) und schlägt optimale Reglerparameter (PI, Feedforward, Boden-Grenzen) vor.
3. **Übernahme:** Die Parameter werden per OTA/NVS auf den ESP32 geflasht → ab dann läuft er **autonom**.

**Optional (Stretch-Goal):** Cloud-Optimierung – wenn Internet verfügbar ist, Daten hochladen und verfeinerte Setpoints zurückbekommen; fällt das Internet aus, greift immer der lokale Regler.

## 6. Systemarchitektur (Gesamtbild)

```
                  ┌────────────────────────────────────────────┐
 Außentemperatur ─► Vorsteuerung (Feedforward)                 │
 Raumtemp je Zone─► Zonenregler (PI, langsam) ──► Ventil %     │──► 7 Ventile
 Bodentemp (Modell)► Begrenzung je Zone (Holz 27/Fliese 29)    │
 Wärmebedarf? ────► Master-Logik ──► SSR ──► Hauptventil (230V)│
                  └────────────────────────────────────────────┘
                    Alles lokal auf dem ESP32-S3 (offline-fähig)
```

- **Regelkreis 100 % lokal** auf dem ESP32-S3; kein Internet, kein Cloud-Zwang.
- **MQTT (lokal, Mosquitto)** + optional **Home Assistant** nur für Monitoring/manuelle Übersteuerung – fällt der Broker aus, läuft die Heizung unverändert weiter.
- **Sensoren:** 6 Raumfühler (Funk, Modell noch offen), 1 Vorlauf-DS18B20 (1-Wire), optional 1 Bodenfühler.

## 7. Sicherheit & Betrieb

- **Fail-safe:** definiertes Fehlerverhalten – bei Verbindungsverlust wahlweise „Ventil voll offen" (Frostschutz) oder „letzte Position halten".
- **Watchdog** (HW+SW) und **Laufzeit-Timeouts** als Backup zur Stall-Erkennung.
- **230 V ausschließlich am SSR-Ausgang** (Hauptventil); Logik bleibt Niederspannung.
- Kalibrier-/Konfigdaten persistent im **NVS**.

## 8. Phasenplan

1. ✅ Konzept (dieses Dokument + README)
2. Schaltplan/Trägerplatine (EasyEDA)
3. Firmware-Grundgerüst (PlatformIO + Arduino-ESP32, ESP32-S3)
4. Kalibrier-/Logging-Phase → thermisches Modell + KI-Tuning
5. Inbetriebnahme & Betrieb