# Stückliste (BOM)

Stand: vor Schaltplan. **Bedarf** = was die 7-Zonen-Anlage wirklich braucht.
**Haben** = was du im Bestand hast bzw. schon bestellt hast (LCSC-Nr. in Klammern).

## Kern-Bauteile

| Bauteil | Bedarf | Haben | Status | Verwendung |
|---|---|---|---|---|
| ESP32-S3-DevKitC-1 (S3-WROOM-1-N16R8) | 1 | 1 | ✅ haben | Controller |
| DRV8833CPWPR (HTSSOP-16) | 4 | 4 | ✅ bestellt | Motor-Treiber |
| INA219AIDR (C138706) | 1 | 1 | ✅ haben | Strommessung (Stall, I²C) |
| MCP602-I/SN (C29605) | 1 | 4 | ✅ haben | Ripple-OpAmp (Doppel-OpAmp, reicht) |
| LM393DR2G (SOIC-8, dual) | 1 | 1 | ✅ bestellt | Ripple-Komparator (Open-Collector) |
| Shunt 100 mΩ / 2 Ω (C4135594 / C1855973) | 1 | 1 | ✅ haben | Ripple-Abgriff |
| SSR (3,3 V, 40 A) | 1 | 1 | ✅ haben | Hauptventil (230 V) |
| ME6217C33M5G / MSMP2307DN-LF-Z | 1 | 1 | ✅ haben | 5 V → 3,3 V_MOTOR |
| AP63205WU-7 + 10 µH (C2071056 / C514807) | 1 | 1 | ✅ haben | 5-V-Buck (optional 12 V → 5 V) |
| Würth 615006138421 (RJ12 6P6C) | 7 | 7 | ✅ bestellt | Ventil-Anschluss |
| DS18B20-Anlegefühler | 1 | 0 | 🛒 kaufen | Vorlauftemperatur |

## Treiber, Schutz & Passives

| Bauteil | Bedarf | Haben | Status | Verwendung |
|---|---|---|---|---|
| SS8050 (C7420370) | 2 | 50 | ✅ haben | SSR-/Buzzer-Treiber |
| PTC-Sicherung 1812L300 (C20627123) | 1 | 10 | ✅ haben | Eingangsschutz |
| Buzzer MLT-8540H (C95298) | 1 | 5 | ✅ haben | Fehler-Signalton |
| Dioden (BAT54W, 1N4148W, 1N5817WS, …) | div. | div. | ✅ haben | Schutz / Freilauf |
| Widerstände (1k, 1k5, 2k2, 10k, 47k, 100k, …) | div. | div. | ✅ haben | Pullups / Filter / Hysterese |
| Kondensatoren (100 n, 10 µ, 22 µ, 100 µ, 470 µ) | div. | div. | ✅ haben | Entkopplung / Bulk |
| XH2.54-Stecker (2/3/4/6-pol) | div. | div. | ✅ haben | Anschlüsse |
| AO3401A / AO3400 / MMBT5551 | div. | div. | ✅ haben | Schalter / Treiber |

## Zusammenfassung – noch zu beschaffen

| Bauteil | Menge | ca. Preis |
|---|---|---|
| DS18B20-Anlegefühler | 1 | ~3 € |
| externes 5-V-Netzteil | 1 | ~5 € (falls nicht vorhanden) |

## Noch offen

- Raumfühler (6×): Funk-Sensor, Modell offen (siehe `docs/sensorik.md`).
- Bodensensor (0–1×): nur falls Vorlauf > ~32 °C.
