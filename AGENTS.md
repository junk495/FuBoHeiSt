# AGENTS.md – FuBoHeiSt

Leitfaden für KI-Agenten und Mitentwickler. Vor jeder Aufgabe vollständig lesen.
Die inhaltliche Spec steht in `README.md` – dieses Dokument beschreibt, **wie** hier gearbeitet wird.

## Worum geht es?

FuBoHeiSt ist eine lokale, offline-fähige Steuerung der Fußbodenheizung einer Wohnung:

- **7 HmIP-VDMOT-Stellmotoren** (3,3 V DC, Richtung über Umpolung) über DRV8833-H-Brücken
- **1 Hauptventil** (motorisierter Kugelhahn, 230 V, ein/aus) über ein SSR
- Regelkreis (Raum-/Außen-/Bodentemperatur) läuft **komplett lokal** auf dem ESP32 – kein Cloud-/Internet-Zwang

## Eingefrorene Entscheidungen (Details in `README.md`)

- Controller: **ESP32-S3-DevKitC-1** (ESP32-S3-WROOM-1-N16R8) als Steckmodul auf einer Trägerplatine.
- Motortreiber: **4× DRV8833** (7 Kanäle + 1 Reserve), Topologie „Variante A" (1 H-Brücke je Motor, 1 geteilte Messkette).
- Strommessung: **INA219** (I²C) für Stall-Erkennung; externer **Shunt → MCP602 → LM393 → PCNT** für Ripple-Counting.
- Versorgung: **extern 5 V** → PCB (**ME6217** oder **MP2307** → 3,3 V_MOTOR); das DevKit erzeugt sein eigenes 3,3 V aus 5 V.
- Hauptventil: **SSR**, 3,3 V direkt ansteuerbar, 40 A.
- Firmware: **PlatformIO + Arduino-ESP32 (C++)**.

## Konventionen

- Doku & Kommentare: **Deutsch**. Code-Identifier/Funktionsnamen: **Englisch**.
- **Nicht raten oder interpretieren:** nur dokumentierte Fakten verwenden; bei Unklarheit nachfragen.
- Entscheidungen ändern → immer `README.md` (Spec) nachziehen.

## Struktur

```
FuBoHeiSt/
├── README.md      Spec (Ziel, Architektur, BOM, Datenmodell, TODO)
├── AGENTS.md      dieses Dokument
├── .clinerules    Cline-spezifische Regeln
├── docs/          Schaltplan-/Netzlisten-Konzept, Pinbelegung, weitere Doku
├── hardware/      EasyEDA-Schaltplan/-PCB
└── firmware/      PlatformIO-Projekt
```

## Sicherheit (unbedingt)

- **230 V ausschließlich am SSR-Ausgang** (Kugelhahn). Die Logik bleibt Niederspannung.
- Mains-Isolation nicht umgehen; keine 230-V-Strecke auf der Trägerplatine außer am SSR.
