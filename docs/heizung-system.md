# Heizungssystem – Faktenaufnahme (Regelstrecke)

> **Zweck:** Reine, dokumentierte Aufzeichnung der **Heizungsanlage** und ihrer
> Zusammenhänge (Vorlauf, Hauptventil, Verteiler, Heizkreise, Stellmotoren,
> Räume, Sensoren). Dieses Dokument ist die **Basis für die Regelungslogik**.
>
> **Regel:** Hier stehen nur **gesicherte Fakten** (Quelle in Klammern).
> Unklares wird ausdrücklich als **[offen]** markiert – nicht geraten.
> Die elektronische Umsetzung (Pins, Schaltung) steht bewusst *nicht* hier,
> sondern in `README.md` und `docs/schaltung/` (00–08).

---

## 1. Systemüberblick & Systemgrenzen

FuBoHeiSt regelt eine **Fußbodenheizung (FBH) einer Wohnung** mit **7 Heizkreisen**.

**Was geregelt wird (Aktoren):**

| Ebene | Aktor | Stellgröße | Quelle |
|---|---|---|---|
| Einzelne Heizkreise | 7× HmIP-VDMOT-Stellmotor | 0–100 % Öffnung (hydraulischer Abgleich) | `README.md` §1, `gesamtkonzept.md` §2 |
| Gesamtdurchfluss | 1× Hauptventil (motorisierter Kugelhahn, 230 V) | ein/aus | `README.md` §1, `gesamtkonzept.md` §2 |

**Was *nicht* geregelt wird:**

- **Vorlauftemperatur** → wird vom Gebäude festgelegt, kein Eingriff durch FuBoHeiSt (`README.md` §1, `gesamtkonzept.md` §2).

**Grundsatz:** Regelkreis läuft komplett lokal auf dem ESP32-S3, offline-fähig (`README.md`, `AGENTS.md`).

### 1.1 Vereinfachtes Wirkungsbild

```
                    (vom Gebäude festgelegt)
   Wärmeerzeuger ──► VORLAUF ──► [Hauptventil 230V ein/aus] ──► VERTEILER
                                     (Gesamtdurchfluss)            │
                                                                   ├─► Heizkreis 1 ──[Stellmotor 0–100%]──► Raum/Boden (Zone 1)
                                                                   ├─► Heizkreis 2 ──[Stellmotor]────────► Raum/Boden (Zone 2)
                                                                   ├─► ...
                                                                   └─► Heizkreis 7 ──[Stellmotor]────────► Raum/Boden (Zone 7)
                                                                                                   │
   Raum/Boden ──► RÜCKLAUF ──────────────────────────────────────────────────────────────────────────┘
```

> Das Hauptventil sitzt im **Vorlauf vor dem Verteiler** (Details §2.2); ein **manueller
> Kugelhahn** dient zusätzlich als Absperrung. Noch offen: Fail-Verhalten bei Stromausfall (§8).

---

## 2. Bausteine der Regelstrecke & Zusammenhänge

### 2.1 Vorlauf

- Der **Vorlauf** ist das heiße Wasser, das in die Heizkreise fließt.
- **Nicht geregelt:** FuBoHeiSt greift nicht in die Vorlauftemperatur ein (`gesamtkonzept.md` §2).
- **Wettergeführt vom Gebäude:** Die Hausheizung regelt die Vorlauftemperatur über einen
  **eigenen Außentemperaturfühler** – außen wärmer → Vorlauf geringer, außen kälter → Vorlauf höher.
- **Max. gemessene Vorlauftemperatur: ≈ 32 °C** (Messung des Betreibers; ≈ an der Schwelle für
  die Bodensensor-Entscheidung, siehe `gesamtkonzept.md` §4).
- **Messung (geplant, gesamt 8× DS18B20):**
  - 1× DS18B20-Anlegefühler am **Haupt-Vorlaufrohr** (vor dem Verteiler).
  - 7× DS18B20 – je **1 nach jedem Kreis-Ventil** → Vorlauftemperatur je Heizkreis.

> **[offen]** Genaue Heizkurve/Heizgrenze der Hausheizung – siehe Fragen §8.

### 2.2 Hauptventil (Gesamtdurchfluss)

- **Position:** im **Vorlauf vor dem Verteiler**, vor den einzelnen Kreis-Ventilen →
  regelt den **Gesamtdurchlauf** aller 7 Kreise (Angabe Betreiber).
- **Typ (Etikett):** Motorisiertes Zonenventil **„MOD. SF 20-2"** (ART. 7.001.02054), **230 V 50/60 Hz, 5 W, 0,04 A**.
- **Anschluss (3-Leiter):** **braun** (L), **blau** (N), **grün-gelb** (PE/Schutzleiter) → 2-Punkt auf/zu.
- **Handnotbetätigung:** Schiebehebel mit Stellungen **AUTO / MAN**.
- **Kennwerte:** max. stat. Druck 10 bar (980 kPa), max. Differenzdruck 0,92 bar (90,2 kPa), Fluid bis 110 °C, Umgebung bis 60 °C.
- **Stellverhalten:** auf/zu (Betreiber); Steuerung über **SSR** (3,3 V, 40 A) (`README.md` §3).
- Zusätzlich vorhanden: **manueller Kugelhahn** (Flügelgriff, vertikal = offen) als Absperrung.
- Master-Logik (festgelegt): **EIN**, sobald ≥1 Zone Wärmebedarf hat; **AUS** bei keinem Bedarf (mit Nachlauf) (`README.md` §4.3, `gesamtkonzept.md` §3).

> **[offen]** Fail-Verhalten bei Stromausfall (letzte Position vs. Rückstellung) und konkrete
> Nachlauf-/Frostschutzwerte – werden bei der Steuerung festgelegt (§8).

### 2.3 Verteiler (Heizkreisverteiler)

- **1 Verteilerkasten** mit **2 Balken**: oben **rot = Vorlauf**, unten **blau = Rücklauf** (Angabe Betreiber).
- **7 Heizkreise**, je Kreis ein Ventil (am Vorlauf-Balken) mit **Stellmotor**.
- **Durchflussmesser (Topmeter/Schaugläser)** am Rücklauf-Balken – **nur visuell**, nicht in die Steuerung integrierbar.
  Aktuell **verstellt** (nicht korrekt eingestellt), aber **manuell einstellbar**.
- **Hydraulischer Abgleich:** die **Stellmotoren** ersetzen die **manuellen Stellräder** auf den
  Vorlauf-Ventilen und übernehmen die Durchfluss-Einstellung je Kreis (Angabe Betreiber).
- Am Rücklauf-Hauptstrang: **Strangregulier-/Differenzdruckventil** – **Einstellung unbekannt**, Betreiber kann dort nichts einstellen.
- **Ist-Zustand:** aktuell **1 Stellantrieb** montiert (Kreis „Bad") + **6 manuelle Stellräder**
  (WZ1, WZ2, KÜ, Gang, SZ2, SZ1); **alle manuellen Stellräder sollen durch Stellmotoren ersetzt werden**.

### 2.4 Heizkreise & Stellmotoren

- **7 Heizkreise**, je Kreis **1 HmIP-VDMOT-Stellmotor** (`README.md` §1, `gesamtkonzept.md` §1).
- Zusätzlich **1 Reserve-Kanal** (8. Motor, voll verkabelt, kein fester Heizkreis) für Hot-Swap bei Kanal-Ausfall (`README.md` §4.1, `gesamtkonzept.md` §2).
- **Stellmotor = „dummer" DC-Getriebemotor** (HmIP-VDMOT):
  - 3,3 V DC, **Blockierstrom (Stall) ≈ 120 mA** (`README.md` §1, `gesamtkonzept.md` §1).
  - **Richtung über Umpolung** (H-Brücke) (`README.md` §1).
  - **Endanschlag über Stall-Detection** (Stromanstieg), **Zwischenpositionen über Ripple-Counting** (Kommutator-Pulse zählen) (`README.md` §1, `gesamtkonzept.md` §3).
  - **Getriebe selbsthemmend** → **kein Haltestrom** nötig (`README.md` §1, `gesamtkonzept.md` §1).
- **Nur 1 Motor läuft gleichzeitig** (Round-Robin) (`README.md` §3.1, `gesamtkonzept.md` §1).

**Physikalische Heizkreise am Verteiler** (Reihenfolge links → rechts, Label + Rohrlänge):

| Nr. | Label | Rohrlänge | Raumzuordnung |
|---|---|---|---|
| 1 | WZ1 | 85 m | Wohnzimmer |
| 2 | WZ2 | 85 m | Wohnzimmer |
| 3 | KÜ | 80 m | Küche |
| 4 | Gang | 60 m | Vorraum |
| 5 | Bad | 55 m | Bad + WC |
| 6 | SZ2 | 100 m | Kinderzimmer |
| 7 | SZ1 | 90 m | Schlafzimmer |

### 2.5 Rücklauf

- Der **Rücklauf** ist der untere (blaue) Balken; er führt das abgekühlte Wasser zurück.
- Am Rücklauf-Hauptstrang sitzt ein **Strangregulier-/Differenzdruckventil** (Einstellung unbekannt).
- **Keine Messung** des Rücklaufs vorgesehen (derzeit nicht regelungstechnisch genutzt).

### 2.6 Hydraulische Kopplung & nutzbarer Stellbereich

- Die 7 Kreise sind **hydraulisch stark gekoppelt**: Wird ein Kreis geöffnet, **sinkt der
  Durchfluss der übrigen Kreise** (Angabe Betreiber).
- Die **Heizschlauchlänge** (55–100 m, siehe §2.4) ist dafür mitentscheidend (Rohrwiderstand je Kreis).
- Daraus folgt: der **tatsächlich nutzbare Regelbereich** der Stellmotoren liegt **innerhalb
  weniger Prozent des möglichen Stellwegs** (Angabe Betreiber):
  - **Größenordnung:** ca. **2–5 %** des Stellwegs (Schätzung des Betreibers).
  - **Lage:** eher **nahe „zu"** (Ventil kaum geöffnet).
  - **Basis:** Beobachtung am Durchfluss-Schauglas.
- **Umwälzpumpe:** **unbekannt**, vermutet **konstant** (beeinflusst die Kopplungsstärke).


---

## 3. Zonen, Räume & Heizkreise

**Reale Zuordnung am Verteiler** (Angabe Betreiber) – 7 Heizkreise, aber die Raum-Struktur
weicht von der früheren 1:1-Annahme ab:

| Kreis | Label | Raum | Belag | Max. Bodentemp. | Raumfühler |
|---|---|---|---|---|---|
| 1 | WZ1 | Wohnzimmer | Holz | 27 °C | WohnKue (geteilt) |
| 2 | WZ2 | Wohnzimmer | Holz | 27 °C | WohnKue (geteilt) |
| 3 | KÜ | Küche | Fliesen | 29 °C | WohnKue (geteilt) |
| 4 | Gang | Vorraum | Fliesen | 29 °C | Gang |
| 5 | Bad | Bad + WC | Fliesen | 29 °C | Bad **und** WC |
| 6 | SZ2 | Kinderzimmer | Holz | 27 °C | KiZi |
| 7 | SZ1 | Schlafzimmer | Holz | 27 °C | Schlafz |

**Strukturelle Besonderheiten (wichtig für die Regelungslogik):**

- **Wohnküche:** Die 3 Kreise **WZ1, WZ2, KÜ** liegen in **einem großen Raum** und teilen sich
  **1 Raumfühler** → 3 Ventile, 1 Istwert (aber unterschiedliche Boden-Grenzen: Holz 27 °C / Fliese 29 °C).
- **Bad + WC:** Der **eine** Kreis „Bad" versorgt **zwei getrennte Räume** (Bad und WC) mit
  **je eigenem Raumfühler** → 1 Ventil, 2 Istwerte / 2 Sollwerte.
- **6 Raumfühler insgesamt**, **7 Ventile** (+ 1 verkabelter Reserve-Kanal).

> Die frühere Zonen-Tabelle in `README.md` §2 (Bad=1, WC=2, …) wurde durch die reale
> Kreis-Zuordnung ersetzt (siehe `README.md` §2).

---

## 4. Messgrößen / Sensoren (Eingänge der Regelung)

| Messgröße | Sensor | Anzahl | Zweck | Quelle |
|---|---|---|---|---|
| Raumtemperatur je Raum | BLE-Raumfühler (BTHome v2) | 6 | Istwert der Regelung | `README.md` §2, `sensorik.md` |
| Vorlauftemperatur (Haupt-Vorlauf) | DS18B20-Anlegefühler (1-Wire) | 1 | Regelung + Holz-Schutz-Entscheidung | `README.md` §2, `schaltung/06-1-wire.md` |
| Vorlauftemperatur je Kreis | DS18B20 (1-Wire) | 7 (geplant) | Ist-Vorlauf je Heizkreis | Anforderung Betreiber |
| Bodentemperatur | — (modellbasiert) | 0 | Boden-Begrenzung | `README.md` §2, `gesamtkonzept.md` §4 |
| Bodentemperatur (temporär, Entwicklung) | DS18B20-Bodenfühler | 0 dauerhaft | Modell-/Simulationsphase (erwogen) | Betreiber |
| Außentemperatur | — (kein eigener Sensor) | 0 | ggf. Vorsteuerung | Betreiber (siehe unten) |

> **Außentemperatur:** Es gibt **keinen eigenen** Außentemperaturfühler. Die Hausheizung nutzt
> ihren eigenen Fühler nur zur **Vorlauf-Führung**. Ob wir die Außentemperatur für eine
> Vorsteuerung nutzen, wird **bei der Steuerung entschieden** (§8).

**Zuordnung BLE-Raumfühler ↔ Raum/Kreis** (MAC-Adressen, aktuell in Firmware-Konfig):

| Fühler-Name | MAC (BLE) | Raum | Kreis(e) |
|---|---|---|---|
| WohnKue | `A4:C1:38:C2:92:A7` | Wohnküche (WZ + Küche) | WZ1, WZ2, KÜ |
| Gang | `A4:C1:38:A1:EE:FD` | Vorraum | Gang |
| Bad | `A4:C1:38:36:FB:F1` | Bad | Bad |
| WC | `A4:C1:38:E9:8B:CC` | WC | Bad |
| Schlafz | `A4:C1:38:51:E4:5B` | Schlafzimmer | SZ1 |
| KiZi | `A4:C1:38:E0:BA:0F` | Kinderzimmer | SZ2 |

Quelle: `firmware/include/config.h`.

---

## 5. Thermische / physikalische Fakten (relevant für die Regelung)

- **FBH ist sehr träge:** Zeitkonstanten im Stundenbereich, große Totzeit → naives PID würde schwingen (`gesamtkonzept.md` §3).
- **Bodentemperatur kann nie über die Vorlauftemperatur steigen** (physikalischer Fakt) → Holz-Schutz über Vorlauf-Messung ableitbar (`gesamtkonzept.md` §4).
- **Bodentemperatur folgt dem Ventil nur verzögert** (τ ≈ 45–90 min) → Bodenmodell als **zeitdiskretes RC-Glied** (exponentiell geglätteter Zustand), kein statisches `f(...)` (`README.md` §4.3, `gesamtkonzept.md` §4).
- **Boden-Grenzwerte je Zone:** Holz 27 °C, Fliesen 29 °C (Bad bis ~33 °C) → Ventil drosseln (`README.md` §2, §4.3).
- **Getriebe selbsthemmend** → kein Haltestrom; Motor fährt nur bei Stellungsänderung (`README.md` §1).
- **Wettergeführter Vorlauf** (Hausheizung): außen wärmer → Vorlauf geringer, außen kälter → Vorlauf höher (Angabe Betreiber).
- **Unterschiedliche Kreis-Rohrlängen** (55–100 m) → je Kreis leicht unterschiedliche thermische Strecke (fürs Boden-/Raummodell relevant).
- **Hydraulische Kopplung** der Kreise: Öffnen eines Kreises senkt den Durchfluss der anderen; nutzbarer Stellbereich nur wenige Prozent des Stellwegs (§2.6).

---

## 6. Festgelegte Regelstrategie (Fakten, auf denen die Logik aufbaut)

- **Langsame Regelung:** Abtastzeit **~60 s**, Ausgang **ratenbegrenzt**, **PI mit Anti-Windup** (`README.md` §4.3, `gesamtkonzept.md` §3).
- **Vorsteuerung über Außentemperatur** (Feedforward/Heizkurve) – antizipiert Wärmebedarf (`README.md` §4.3, `gesamtkonzept.md` §3).
- **Bodentemperatur-Begrenzung je Zone** (Holz 27 °C / Fliesen 29 °C) → Ventil drosseln (`README.md` §4.3).
- **Frostschutz mit Vorrang** (`README.md` §4.3, `gesamtkonzept.md` §3).
- **Master-Logik:** Hauptventil EIN bei ≥1 Zone offen, AUS bei keinem Bedarf (mit Nachlauf) (`README.md` §4.3, `gesamtkonzept.md` §3).
- **Positionsregelung je Ventil:** Endanschlag = Stall (Strom), Zwischenposition = Ripple-Zählung;
  Kalibrierung „ganz zu" = 0 Pulse, „ganz auf" = Gesamtpulse → Position in % (`README.md` §4.2).
- **Auto-Homing** gegen Ripple-Drift: Soll ≤5 % oder ≥95 % → bis Stall referenzieren; nach ~50 Teilbewegungen ohne Endanschlag einmal referenzieren (`README.md` §4.2).

**Hinweis (Struktur):** Die Regelung muss zwei Sonderfälle abbilden:
- **Wohnküche:** 3 Ventile (WZ1/WZ2/KÜ) teilen 1 Raumfühler → Koordination der 3 Ventile + unterschiedliche Boden-Grenzen.
- **Bad/WC:** 1 Ventil versorgt 2 Räume mit 2 Fühlern → Strategie für einen gemeinsamen Ventilstellwert nötig (offen).

---

## 7. Datenmodell (Auszug, für die Regelung relevant)

MQTT-Topics (lokal, z. B. Mosquitto) – Auszug aus `README.md` §5.1:

```
fbh/zone/<id>/target_temp      ← Soll-Raumtemperatur
fbh/zone/<id>/current_temp     → Ist-Raumtemperatur
fbh/zone/<id>/valve_target     → Ziel Ventil-% (vom Regler)
fbh/zone/<id>/valve_position   → Ist Ventil-%
fbh/zone/<id>/floor_temp       → (optional)
fbh/zone/<id>/status           → idle/moving/calibrating/error
fbh/master/flow                → ein/aus
fbh/supply/temp                → Vorlauftemperatur °C
fbh/config/...                 → Parameter
```

Persistente Konfig (NVS) – Auszug aus `README.md` §5.2:

- Zonenliste: Name, Belag, `max_Bodentemp`, `P_ges`, `P_touch`, Totband
- Reglerparameter je Zone (PI + Vorsteuerung)
- Master-Logik (Nachlauf, Frostschutz-Schwelle)

---

## 8. Offene Punkte / Fragen an den Betreiber

**Geklärt (Stand aktuell):**
- Hauptventil: Position (Vorlauf vor Verteiler), Typ „SF 20-2" (230 V, 5 W, 0,04 A, 3-Leiter L/N/PE, AUTO/MAN).
- 1 Verteiler (rot = Vorlauf / blau = Rücklauf), 7 Kreis-Labels + Rohrlängen, Durchflussmesser nur visuell (aktuell verstellt, manuell einstellbar).
- Vorlauf wettergeführt (Hausheizung); **max. gemessene Vorlauftemperatur ≈ 32 °C**.
- Kreis → Raum: SZ1 = Schlafzimmer, SZ2 = Kinderzimmer (beide Holz 27 °C).
- Sensoren: 8× DS18B20 (1 Haupt-Vorlauf + 7 je Kreis), alle parallel am 1-Wire-Bus.
- Hydraulische Kopplung: nutzbarer Stellbereich ca. 2–5 % des Stellwegs, eher nahe „zu" (Beobachtung am Schauglas).
- Hydraulischer Abgleich: **Stellmotoren ersetzen die 6 manuellen Stellräder** und übernehmen den Abgleich.
- Bodensensor: **kein permanenter Bodensensor** (Entscheidung); temporär für Entwicklung erwogen.

**Noch offen / bei der Steuerung zu entscheiden:**

1. **Bad/WC-Regelung:** 1 Ventil versorgt 2 Räume mit getrennten Fühlern/Sollwerten – Strategie für den gemeinsamen Ventilstellwert.
2. **Wohnküche-Regelung:** WZ1/WZ2/KÜ immer gleich fahren oder unabhängig (nur unterschiedliche Boden-Grenzen)?
3. **Außentemperatur:** Vorsteuerung über Außentemperatur (Quelle nötig) oder Verzicht?
4. **Hauptventil-Fail-Verhalten:** Stellung bei Stromausfall (letzte Position vs. Rückstellung).
5. **Master-Parameter:** Frostschutz-Schwelle und Nachlaufzeit des Hauptventils.
6. **Holz-Bodentemperatur-Grenzwert:** keine Herstellerangabe vorhanden; bislang unkritisch → als „kein akutes Thema" eingestuft.
7. **Strangregulierventil + Pumpe:** Einstellung/Verhalten unbekannt (beeinflusst die Kopplungsstärke).
8. **Heizkurve/Heizgrenze:** genaue Heizkurve der Hausheizung im Detail (max. Vorlauf ≈ 32 °C ist bekannt).
9. **Temporäre Bodensensoren:** sinnvoll für Entwicklung/Simulation des Bodenverhaltens? (offen)

