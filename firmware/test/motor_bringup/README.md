# Motor-Bringup (Stufe 1)

Eigenständiges Testprojekt zum Prüfen der Motor-Hardware:
DRV8833 (8 Kanäle), INA219 (Strommessung/Stall), Ripple-Zählung (PCNT), DS18B20, Buzzer.

## Flashen

```powershell
cd firmware/test/motor_bringup
pio run -t upload
pio device monitor
```

## Serielle Befehle (115200 Baud)

| Befehl | Funktion |
|---|---|
| `c<N>` | Kanal wählen (1..8, Default 1) |
| `f` | vorwärts fahren bis Stall/Timeout |
| `r` | rückwärts fahren bis Stall/Timeout |
| `s` | Stopp (Brake) |
| `m<Wert>` | Stall-Schwelle setzen (mA), `m` = anzeigen |
| `o<Wert>` | Shunt-Widerstand setzen (mOhm), `o` = anzeigen |
| `i` | INA219-Status + nFAULT/PCNT |
| `b` | Buzzer-Piep |
| `t` | DS18B20 lesen |
| `h` | Hilfe |

Kanal 1 = Zone 1 (IN1=GPIO4, IN2=GPIO5). Ziel: `P_touch`, `P_ges` ermitteln.
Gemessen am Motor: frei ~12 mA, blockiert ~98 mA → Stall-Schwelle Default 60 mA.

## Hinweis

- Stall-Erkennung: Nach dem Start gilt eine Blindzeit (400 ms), damit der
  Anlaufstrom (Inrush, laut Doku 180–250 mA) keinen Falsch-Stall auslöst.
  Ohne frischen INA219-Messwert wird nicht entschieden (keine veralteten Werte).
- Die Motor-Module (`MotorDriver`, `CurrentSense`, `RippleCounter`) und `pins.h`
sind **Kopien** aus `firmware/src` bzw. `firmware/include`. Bei Änderungen an den
Originalen diese Kopien hier ggf. nachziehen.

