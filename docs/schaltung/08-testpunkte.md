# 08 – Testpunkte & Debug-Header

> Zweck: Debug-/Diagnose-Zugänge für Inbetriebnahme und Fehlersuche.
> Enthält den UART-Debug-Header (CN-UART) und Messpunkte (TPx) für die wichtigsten Netze.
> RefDes/Netze: siehe `00-referenz.md`.
>
> **Hinweis:** Die Messpunkte TP1–TP14 existieren **nur als PCB-Pads** (kein
> Schaltplan-Symbol, nicht in der Netzliste) – sie werden im PCB direkt auf die
> jeweiligen Netze gelegt.

## 1. UART-Debug-Header (CN-UART)

Zum Debuggen/Loggen über eine externe USB-UART-Adapterkarte. Der Controller ist
pin-technisch voll, daher wird **UART0** genutzt – er ist firmware-seitig frei
(die Firmware loggt aktuell über `Serial` = USB-CDC auf GPIO19/20).

| Pin | Netz | GPIO | Footprint-Pin (DevKitC-1) | Richtung |
|---|---|---|---|---|
| 1 | GND | — | — | — |
| 2 | RXD | GPIO44 (U0RXD) | 42 | in |
| 3 | TXD | GPIO43 (U0TXD) | 43 | out |
| 4 | +3V3 | — | — | Versorgung |

**Stecker:** XH2.54 4-pol (wie CN1–CN12).

> **Wichtig – CH343P-Konflikt:** GPIO43/44 sind auf dem DevKitC-1 über den
> On-Board-Chip **CH343P** auf die **rechte USB-C-Buchse** geführt. Ein externer
> Adapter am CN-UART und der CH343P hängen damit parallel an derselben Leitung.
> Reines Abhören (nur TX) ist unkritisch; für bidirektionales Debugging darf nur
> **eine** Quelle gleichzeitig angeschlossen sein (TXD/RXD am Adapter kreuzen:
> ESP32-TXD → Adapter-RXD). Ob sich der CH343P per 0-Ω-Brücke trennen lässt, ist
> am Board zu prüfen (nicht dokumentiert).

## 2. Testpunkte – volle Version

| Ref | Netz | Beschreibung | Warum |
|---|---|---|---|
| TP1 | +5V | 5-V-Eingang (nach PTC F1) | Versorgungs-Check |
| TP2 | +3V3 | Logik-3,3 V (DevKit) | Pullups/INA219-VS |
| TP3 | +3V3_MOTOR | Motor-3,3 V (ME6217-Ausgang) | Motorversorgung vor Shunt |
| TP4 | GND | gemeinsame Masse | Referenz für alle Messungen |
| TP5 | I2C_SDA | I²C-Daten (GPIO1) | Bus-Analyse am INA219 |
| TP6 | I2C_SCL | I²C-Takt (GPIO2) | Bus-Analyse am INA219 |
| TP7 | nFAULT | DRV8833 wire-AND (GPIO21, LOW=Fehler) | erster Blick bei Motor-Problemen |
| TP8 | nSLEEP | DRV8833-Enable (GPIO18, HIGH=aktiv) | Treiber-Zustand |
| TP9 | ISEN | Low-Side-Shunt-Knoten (alle AISEN/BISEN) | Spannung ∝ Motorstrom |
| TP10 | TP_FILT | Tiefpass-Ausgang (U7.3 IN1+) | analoge Messkette vor Verstärker |
| TP11 | MCP_OUT | Verstärker-Ausgang (U7.1, Gain 23) | Ripple-Signal nach Gain |
| TP12 | RIPPLE | LM393-Ausgang → PCNT (GPIO38) | Kommutator-Pulse (Positionszählung) |
| TP13 | SSR_CTRL | Hauptventil-Ansteuerung (GPIO47) | SSR-Schaltzustand |
| TP14 | 1WIRE | DS18B20-Bus (GPIO40) | Vorlauf-/Bodentemperatur |

## 3. Testpunkte – kompakte Version (Minimal-Set)

Für eine erste Inbetriebnahme reichen diese Punkte; der Rest lässt sich bei
Bedarf über die übrigen Pads nachrüsten.

| Ref | Netz | Warum |
|---|---|---|
| CN-UART | TXD / RXD | Debug-/Log-Ausgabe (4-pol Header) |
| TP7 | nFAULT | Motor-Fehler (wire-AND) |
| TP8 | nSLEEP | Treiber aktiv/inaktiv |
| TP9 | ISEN | Motorstrom-Messung |
| TP1–TP4 | +5V / +3V3 / +3V3_MOTOR / GND | Versorgungs-Rails |

## 4. Hinweise

- **Kein JTAG verfügbar:** Die JTAG-Pins GPIO39–42 sind bereits belegt
  (39 = Buzzer, 40 = 1-Wire, 41/42 = Motor 8). Breakpoint-Debugging ist nicht
  möglich – Debuggen läuft über **Serial-Logging** (USB-CDC und/oder CN-UART).
- Testpunkte als **gelötete Pads** (z. B. Ø 1,0 mm) nahe am jeweiligen Bauteil
  platzieren; TP4 (GND) mehrfach setzen, damit jede Messung eine kurze
  Masse-Referenz hat.
- Analoge Messpunkte (TP9–TP11) **fern vom SSR** anordnen (siehe `../pcb-design.md` §1).
- Netznamen `TXD`/`RXD` und RefDes `CN-UART` sind in `00-referenz.md` ergänzt.
