# Stückliste (BOM)

Stand: vor Schaltplan. „vorhanden" = aus deinem Bestand (LCSC-Teilenummern in Klammern).

## Vorhanden (Bestand)

| Bauteil | Menge | Verwendung |
|---|---|---|
| ESP32-S3-DevKitC-1 (S3-WROOM-1-N16R8) | 1 | Controller |
| INA219AIDR (C138706) | 1 | Strommessung (Stall, I²C) |
| SSR (3,3 V, 40 A) | 1 | Hauptventil |
| ME6217C33M5G (C427602) / MSMP2307DN-LF-Z (C49375228) | 1 | 5 V → 3,3 V |
| Shunt 100 mΩ (C4135594) / 2 Ω (C1855973) | 1 | Ripple-Abgriff |
| Passives R/C, Dioden, Induktivitäten | div. | überall |
| MMBT5551 / AO3400 / AO3401 u. a. | div. | Treiber/Schalter |

## Kaufen

| Bauteil | Menge | ca. Preis | Verwendung |
|---|---|---|---|
| DRV8833 | 4 | ~1,5–2 €/Stk | Motor-Treiber |
| MCP6002 | 1 | ~0,5 € | Ripple-OpAmp |
| TLV3201 | 1 | ~0,7 € | Ripple-Komparator |
| RJ12/6P6C-Buchse | 7 | ~0,3 €/Stk | Ventil-Anschluss |
| DS18B20-Anlegefühler | 1 | ~3 € | Vorlauftemperatur |
| externes 5-V-Netzteil | 1 | ~5 € | Versorgung (falls nicht vorhanden) |

## Noch offen

- Raumfühler (6×): Funk-Sensor, Modell offen (siehe `docs/sensorik.md`).
- Bodensensor (0–1×): nur falls Vorlauf > ~32 °C.
