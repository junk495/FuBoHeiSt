# 03 – Strommessung (INA219, High-Side)

> Zweck: DC-/Stall-Strom über INA219 (I²C) am High-Side-Shunt (0,5 Ω).
> RefDes/Netze: siehe `00-referenz.md`.

## 1. Übersicht

```
+3V3_MOTOR ─► U6.8 (IN+) ─[R17∥R18 = 0,5 Ω]─ U6.7 (IN−) ─► VM (DRV8833)
                    U6 (INA219) ─► I2C_SDA / I2C_SCL ─► GPIO1 / GPIO2
```

## 2. Bauteilliste

| Ref | Bauteil / Wert | Gehäuse | LCSC | Funktion |
|---|---|---|---|---|
| U6 | INA219AIDR | SOIC-8 | C138706 | Strom-/Leistungs-Monitor |
| R17, R18 | 1 Ω (1 %), 2× parallel | 0805 | — | Shunt 0,5 Ω |
| R10 | 4,7 kΩ | 0805 | — | I²C-Pullup SDA |
| R11 | 4,7 kΩ | 0805 | — | I²C-Pullup SCL |
| C15 | 100 nF | 0805 | — | Abblocken VS |

## 3. INA219 – Pinbelegung (SOIC-8)

| Pin | Name | Funktion | Anschluss |
|---|---|---|---|
| 1 | A1 | Adresse | GND (→ 0x40) |
| 2 | A0 | Adresse | GND (→ 0x40) |
| 3 | SDA | I²C-Daten | I2C_SDA (GPIO1) |
| 4 | SCL | I²C-Takt | I2C_SCL (GPIO2) |
| 5 | VS | Versorgung | +3V3 |
| 6 | GND | Masse | GND |
| 7 | IN− | Shunt − | VM (nach Shunt) |
| 8 | IN+ | Shunt + | +3V3_MOTOR (vor Shunt) |

## 4. Netzliste

| Netz | Pins |
|---|---|
| +3V3_MOTOR | U6.8 (IN+), R17.1, R18.1 |
| VM | U6.7 (IN−), R17.2, R18.2 |
| +3V3 | U6.5 (VS), C15.1, R10.1, R11.1 |
| GND | U6.6 (GND), U6.2 (A0), U6.1 (A1), C15.2 |
| I2C_SDA | U6.3 (SDA), R10.2 |
| I2C_SCL | U6.4 (SCL), R11.2 |

## 5. Hinweise

- Shunt = **0,5 Ω** (2× 1 Ω parallel). 120 mA → 60 mV; Anlauf-Peak ~180–250 mA → 90–125 mV.
- Firmware: INA219-PGA **÷4 (±160 mV)** oder **÷8 (±320 mV)** – nicht ÷2 (Sättigung beim Anlauf).
- Adresse 0x40 (A0 = A1 = GND).
- Der INA219 hat **keinen internen Shunt** – der 0,5-Ω-Shunt wird extern platziert.
