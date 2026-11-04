# 00 – Referenz: RefDes & Netznamen

Gemeinsame Referenz für alle Schaltungs-Gruppendateien (`01-power.md` … `07-controller.md`).
Format wie in `can_bus.md`: Netzliste als `RefDes.Pin`.

## RefDes-Übersicht

| Ref | Bauteil | Gruppe |
|---|---|---|
| U1 | ESP32-S3-DevKitC-1 (S3-WROOM-1-N16R8) | Controller |
| U2–U5 | DRV8833PWPR / DRV8833CPWPR (HTSSOP-16) | Motor-Treiber |
| U6 | INA219AIDR (SOIC-8) | Strommessung |
| U7 | MCP602-I/SN (SOIC-8) | Ripple-Messung |
| U8 | LM393DR2G (SOIC-8) | Ripple-Messung |
| U9 | ME6217C33M5G (SOT-23-5) | Power |
| Q1 | AO3400A (SOT-23) – SSR-Treiber | Ausgänge |
| Q2 | AO3400A (SOT-23) – Buzzer-Treiber | Ausgänge |
| D1 | 1N4148W (SOD-123) – Buzzer-Freilauf | Ausgänge |
| D2 | TVS SMAJ5.0A – Eingangsschutz (opt.) | Power |
| F1 | PTC 1812L300 | Power |
| BZ1 | Buzzer MLT-8540H | Ausgänge |
| CN1–CN7 | XH2.54 2-pol (Zonen 1–7) | Motor-Treiber |
| CN8 | XH2.54 2-pol (Reserve, Motor 8) | Motor-Treiber |
| CN9 | XH2.54 2-pol (5-V-Eingang) | Power |
| CN10 | XH2.54 2-pol (SSR-Steuerung) | Ausgänge |
| CN11 | XH2.54 3-pol (1-Wire/DS18B20) | 1-Wire |

## Netznamen (Power)

| Netz | Bedeutung |
|---|---|
| 5V_IN | roher 5-V-Eingang (CN9) |
| +5V | 5 V nach PTC (F1) |
| +3V3 | Logik-3,3 V (DevKit-3V3-Pin) |
| +3V3_MOTOR | Motor-3,3 V (ME6217-Ausgang) |
| GND | gemeinsame Masse |

## Netznamen (Signale)

| Netz | Bedeutung |
|---|---|
| I2C_SDA / I2C_SCL | I²C-Bus (INA219) |
| RIPPLE | LM393-Ausgang → PCNT (GPIO38) |
| nSLEEP / nFAULT | DRV8833-Steuerung |
| SSR_CTRL | GPIO47 → Q1 (SSR) |
| BUZZER | GPIO39 → Q2 (Buzzer) |
| 1WIRE | GPIO40 → CN11 (DS18B20) |
| ISEN | gemeinsamer AISEN/BISEN-Knoten (Low-Side-Shunt) |
| M1_A/M1_B … M7_A/M7_B, M8_A/M8_B | Motor-Ausgänge zu XH2.54 |
