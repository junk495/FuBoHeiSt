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
| `a<N>` | Automatik: N× (r→Endlage, f→Endlage), Log in NVS |
| `d` | Log aus NVS auslesen |
| `k<N>` | Ventil N kalibrieren (Referenzlauf, Hub speichern) |
| `v<N> <0-100>\|a\|z` | Ventil N auf X% schließen (a = auf, z = zu) |
| `K<N>` | **[non-blocking]** Ventil N kalibrieren (Kanal 0..7) |
| `V<N> [0-100]\|a\|z` | **[non-blocking]** Ventil N fahren (Kanal 0..7) |
| `i` | INA219-Status + nFAULT/PCNT |
| `b` | Buzzer-Piep |
| `t` | DS18B20 lesen |
| `h` | Hilfe |

Kanal 1 = Zone 1 (IN1=GPIO4, IN2=GPIO5). Ziel: `P_touch`, `P_ges` ermitteln.
Gemessen am Ventil: Lauf ~17 mA, r-Endlage (hart) ~79 mA, f-Endlage (Gummidichtung) ~61 mA → Stall-Schwelle Default 50 mA.

## Ventillauf (Automatik, ohne Monitor)

Für den Lauf am Ventil (kein Monitor angeschlossen) gibt es einen Auto-Start:

1. In `src/main.cpp` `#define AUTO_RUN_COUNT 0` → z. B. **5** setzen und flashen.
2. Board am Ventil einschalten. Nach `AUTO_RUN_WAIT_S` (10 s) startet die Automatik
   automatisch: je Durchlauf **erst `r` bis Endlage, dann `f` bis Endlage**, und
   schreibt das Ergebnis in den **NVS** (bleibt über Reboot/Re-Flash erhalten).
3. Strom aus, zurück an den PC, Monitor öffnen und `d` eingeben (innerhalb der
   10-s-Wartezeit – jede Eingabe bricht den Auto-Start ab).

Log-Format je Lauf:
`r_pcnt:<n> r_stall:<0/1> r_ma:<mA> f_pcnt:<n> f_stall:<0/1> f_ma:<mA>`

Dazu je Richtung eine Zeitreihe (alle 0,5 s):
`r_ts` bzw. `f_ts` = `pcnt,mA;pcnt,mA;…` (je Sample ein Paar, mit `;` getrennt).

## Ventilansteuerung (Kalibrierung + Position)

Anschläge: **`r` = Kolben-Reverse-Endlage = Ventil AUF (hart)**, **`f` = Kolben-Forward-Endlage = Ventil ZU (Gummidichtung)**.

1. **Kalibrieren (je Ventil):** `k<N>` – fährt f→zu (Dichtung), r→auf (Anschlag), misst den Hub und speichert ihn im NVS (`stroke<N>`).
2. **Positionieren:** `v<N> <X%>` – fährt **direkt von der aktuellen Position** zum Ziel.
   - `v<N>` (ohne Argument) zeigt Position/Hub/Referenzstatus.
   - `v<N> a` = ganz auf, `v<N> z` = ganz zu.
3. Referenz = `r`-Endlage (hart, ~70 mA). Der Hub liegt typisch bei ~6900 Pulsen.
4. Die Position wird im RAM je Kanal verfolgt. Die Referenz wird **nur neu geholt**,
   wenn sie verloren ist (Boot, Timeout/Abbruch/nFAULT). Ein periodisches
   Neu-Kalibrieren (festes Zeitintervall) folgt später.

## Nicht-blockierende Ventilsteuerung (ValveController)

Zusätzlich zu den blockierenden Befehlen ist das **nicht-blockierende** Modul
`ValveController` integriert – identisch zum Modul in der Haupt-Firmware. Es wird in
`loop()` über `valveController.tick()` getrieben (läuft auch ohne Serieleingabe) und
eignet sich für den späteren Regelkreis.

| Befehl | Funktion |
|---|---|
| `K<N>` | [non-blocking] Ventil N kalibrieren (Referenzlauf, Hub speichern) – **Kanal 0..7** |
| `V<N> [0-100]\|a\|z` | [non-blocking] Ventil N auf X% (a = auf, z = zu) – **Kanal 0..7** |
| `V<N>` | [non-blocking] Position/Hub/Referenz anzeigen |

Die Kanalnummerierung ist **0-basiert** (wie in der Haupt-Firmware), im Gegensatz zu den
blockierenden Befehlen `c/k/v` (1-basiert).

## Hinweis

- Stall-Erkennung: Nach dem Start gilt eine Blindzeit (400 ms), damit der
  Anlaufstrom (Inrush) keinen Falsch-Stall auslöst.
  Ohne frischen INA219-Messwert wird nicht entschieden (keine veralteten Werte).
- Die Motor-Module (`MotorDriver`, `CurrentSense`, `RippleCounter`) und `pins.h`
sind **Kopien** aus `firmware/src` bzw. `firmware/include`. Bei Änderungen an den
Originalen diese Kopien hier ggf. nachziehen.

