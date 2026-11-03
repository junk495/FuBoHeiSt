# 07 – Controller (ESP32-S3-DevKitC-1 Pinbelegung)

> Zweck: zentrale Pin-Belegung des Steckmoduls U1 – alle Signale zu den Netzen.
> Vollständige Footprint-Pinout + Hinweise: siehe `../esp32_devkit.md`.

## 1. Pin-Belegung (Firmware-Netze)

| GPIO | Footprint-Pin | Funktion | Netz | Richtung |
|---|---|---|---|---|
| 1 | 41 | I²C SDA | I2C_SDA | bidirektional |
| 2 | 40 | I²C SCL | I2C_SCL | out |
| 4 / 5 | 4 / 5 | Motor 1 IN1/IN2 | M1_IN1 / M1_IN2 | out |
| 6 / 7 | 6 / 7 | Motor 2 IN1/IN2 | M2_IN1 / M2_IN2 | out |
| 8 / 9 | 12 / 15 | Motor 3 IN1/IN2 | M3_IN1 / M3_IN2 | out |
| 10 / 11 | 16 / 17 | Motor 4 IN1/IN2 | M4_IN1 / M4_IN2 | out |
| 12 / 13 | 18 / 19 | Motor 5 IN1/IN2 | M5_IN1 / M5_IN2 | out |
| 14 / 15 | 20 / 8 | Motor 6 IN1/IN2 | M6_IN1 / M6_IN2 | out |
| 16 / 17 | 9 / 10 | Motor 7 IN1/IN2 | M7_IN1 / M7_IN2 | out |
| 41 / 42 | 38 / 39 | Motor 8 IN1/IN2 (Reserve) | M8_IN1 / M8_IN2 | out |
| 18 | 11 | nSLEEP | nSLEEP | out |
| 21 | 27 | nFAULT | nFAULT | in |
| 38 | 35 | Ripple-PCNT | RIPPLE | in |
| 39 | 36 | Buzzer | BUZZER | out (PWM) |
| 40 | 37 | 1-Wire | 1WIRE | bidirektional |
| 47 | 28 | SSR | SSR_CTRL | out |

## 2. Versorgungs-Pins (U1)

| Pin | Netz |
|---|---|
| 5V | +5V |
| 3V3 | +3V3 (Logik-Rail, für Pullups + INA219-VS) |
| GND | GND |

## 3. Netznamen (Motor-Eingänge)

| GPIO | Netz | GPIO | Netz |
|---|---|---|---|
| 4 | M1_IN1 | 12 | M5_IN1 |
| 5 | M1_IN2 | 13 | M5_IN2 |
| 6 | M2_IN1 | 14 | M6_IN1 |
| 7 | M2_IN2 | 15 | M6_IN2 |
| 8 | M3_IN1 | 16 | M7_IN1 |
| 9 | M3_IN2 | 17 | M7_IN2 |
| 10 | M4_IN1 | 41 | M8_IN1 |
| 11 | M4_IN2 | 42 | M8_IN2 |

## 4. Hinweise

- **Gemieden:** Strapping (GPIO0/3/45/46), USB-OTG (19/20), UART0 (43/44), OPI-PSRAM (GPIO33–37, N16R8).
- **JTAG (39–42) frei nutzbar** als normale GPIO (39 = Buzzer, 40 = 1-Wire, 41/42 = Motor 8 Reserve).
- Footprint-Pin-Nummern ≠ GPIO-Nummern (z. B. GPIO40 = Footprint-Pin 37) – im Symbol prüfen.
