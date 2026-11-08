# 06 – 1-Wire (Vorlauftemperatur)

> Zweck: DS18B20-Anlegefühler am Vorlaufrohr (1-Wire-Bus).
> Zwei Stecker am selben Bus: CN11 (Vorlauf) + CN12 (optionaler Bodenfühler).
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
GPIO40 ─► CN11.3 (Daten) ── R12 (4,7 kΩ) ─► +3V3
GPIO40 ─► CN12.3 (Daten)   (selber Bus, optionaler 2. Fühler)
+3V3  ─► CN11.2, CN12.2
GND   ─► CN11.1, CN12.1
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| CN11 | XH2.54 3-pol | THT 2,54 mm | — | 1-Wire (DS18B20, Vorlauf) |
| CN12 | XH2.54 3-pol | THT 2,54 mm | — | 1-Wire (DS18B20, optional Bodenfühler) |
| R12 | 4,7 kΩ | 0805 | — | Pullup |
| DS18B20 | Anlegefühler (extern) | — | — | Vorlauftemperatur |

## 3. Netzliste

| Netz | Pins |
|---|---|
| 1WIRE | R12.1, CN11.3, CN12.3 |
| +3V3 | R12.2, CN11.2, CN12.2 |
| GND | CN11.1, CN12.1 |

## 4. Hinweise

- Pin-Belegung CN11/CN12: **1 = GND, 2 = +3V3, 3 = Daten (1WIRE)**.
- CN12 (optionaler Bodenfühler) liegt am **selben** 1-Wire-Bus – nur falls Vorlauf > ~32 °C.
