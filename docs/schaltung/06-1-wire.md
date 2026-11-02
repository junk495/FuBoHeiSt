# 06 – 1-Wire (Vorlauftemperatur)

> Zweck: DS18B20-Anlegefühler am Vorlaufrohr (1-Wire-Bus).
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
GPIO40 ─► CN10.2 (Daten) ── R12 (4,7 kΩ) ─► +3V3
+3V3  ─► CN10.1
GND   ─► CN10.3
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| CN10 | XH2.54 3-pol | THT 2,54 mm | — | 1-Wire (DS18B20) |
| R12 | 4,7 kΩ | 0805 | — | Pullup |
| DS18B20 | Anlegefühler (extern) | — | — | Vorlauftemperatur |

## 3. Netzliste

| Netz | Pins |
|---|---|
| 1WIRE | R12.1, CN10.2 |
| +3V3 | R12.2, CN10.1 |
| GND | CN10.3 |

## 4. Hinweise

- Pin-Belegung CN10: 1 = +3V3, 2 = Daten (1WIRE), 3 = GND.
- Optional: zweiter DS18B20 (Bodenfühler) an denselben Bus – nur falls Vorlauf > ~32 °C.
