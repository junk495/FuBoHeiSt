# ESP32-S3-DevKitC-1 — Referenz (Modul-Datenblatt)

> Referenzblatt für das Steckmodul **ESP32-S3-DevKitC-1** (U1), Modul **ESP32-S3-WROOM-1-N16R8**
> (16 MB Flash, 8 MB PSRAM, 2,4 GHz WiFi + Bluetooth LE 5).
> Die projektbezogene Pin-Belegung für FuBoHeiSt steht in [`docs/schaltung/07-controller.md`](schaltung/07-controller.md).

## Technische Daten (DevKitC-1)

- Chip: ESP32-S3 (Xtensa LX7 Dual-Core, bis 240 MHz)
- Modul: ESP32-S3-WROOM-1-**N16R8** (16 MB Flash, 8 MB PSRAM)
- Funk: WiFi 802.11 b/g/n + Bluetooth LE 5
- USB: USB-C (USB-Serial-JTAG zum Flashen + `Serial`)
- Spannungsversorgung: **5 V** (USB-C oder 5V-Pin) → Bord-LDO → **3,3 V**
- On-Board: **BOOT-Taster** (GPIO0), **RGB-LED (WS2812, GPIO48)**, RST-Taster

## Pinbelegung (Footprint, 2× Stiftleiste)

### Linke Stiftleiste (Pin 1–22, von oben)

| Pin | Name | Pin | Name |
|---|---|---|---|
| 1 | 3V3 | 12 | GPIO8 |
| 2 | 3V3 | 13 | GPIO3 (Strapping) |
| 3 | RST | 14 | GPIO46 (Strapping) |
| 4 | GPIO4 | 15 | GPIO9 |
| 5 | GPIO5 | 16 | GPIO10 |
| 6 | GPIO6 | 17 | GPIO11 |
| 7 | GPIO7 | 18 | GPIO12 |
| 8 | GPIO15 | 19 | GPIO13 |
| 9 | GPIO16 | 20 | GPIO14 |
| 10 | GPIO17 | 21 | 5V |
| 11 | GPIO18 | 22 | GND |

### Rechte Stiftleiste (Pin 44–23, von oben)

| Pin | Name | Pin | Name |
|---|---|---|---|
| 44 | GND | 33 | GPIO36 |
| 43 | GPIO43 (U0TXD) | 32 | GPIO35 |
| 42 | GPIO44 (U0RXD) | 31 | GPIO0 (BOOT) |
| 41 | GPIO1 | 30 | GPIO45 (Strapping) |
| 40 | GPIO2 | 29 | GPIO48 (RGB-LED) |
| 39 | GPIO42 (JTAG) | 28 | GPIO47 |
| 38 | GPIO41 (JTAG) | 27 | GPIO21 |
| 37 | GPIO40 (JTAG) | 26 | GPIO20 (USB) |
| 36 | GPIO39 (JTAG) | 25 | GPIO19 (USB) |
| 35 | GPIO38 | 24 | GND |
| 34 | GPIO37 | 23 | GND |

## Belegte Pins – GrabGeist (anderes Projekt, nur Referenz)

| Footprint-Pin | GPIO | Funktion |
|---|---|---|
| 1 | 3V3 | Versorgung (+3V3-Netz) |
| 4 | GPIO4 | Servo rechts (PWM) |
| 5 | GPIO5 | Servo links (PWM) |
| 6 | GPIO6 | Akku-Teiler (ADC1_CH5) |
| 7 | GPIO7 | GPIO-Leiste |
| 12 | GPIO8 | SG90-Servo (PWM) |
| 15 | GPIO9 | GPIO-Leiste |
| 16 | GPIO10 | GPIO-Leiste |
| 21 | 5V | Versorgung (+5V-Netz) |
| 22 | GND | Masse |
| 27 | GPIO21 | CAN-D |
| 28 | GPIO47 | CAN-R |
| 35 | GPIO38 | Buzzer |
| 40 | GPIO2 | GPIO-Leiste |
| 41 | GPIO1 | Akustik-Trigger (ADC1_CH0) |
| 42 | GPIO44 (U0RXD) | UART RX |
| 43 | GPIO43 (U0TXD) | UART TX |

## Hinweise & Hardware-Restriktionen (N16R8)

- **SPERRZONE Octal-PSRAM (N16R8):** **GPIO35 (Pin 32), GPIO36 (Pin 33) und GPIO37 (Pin 34)** sind beim N16R8-Modul intern fest für den 8-MB-Octal-PSRAM verdrahtet. **Niemals extern beschalten!**
- **Strapping-Pins (beim Boot kritisch, meiden):** GPIO0 (Pin 31), GPIO3 (Pin 13), GPIO45 (Pin 30), GPIO46 (Pin 14).
- **Kein Hardware-DAC:** Der ESP32-S3 besitzt keinen echten DAC. Analoge Referenzspannungen bei Bedarf per `LEDC-PWM` + RC-Tiefpass (z. B. 10 kΩ + 100 nF) erzeugen.
- **ADC1 vs. ADC2:** 
  - **ADC1 (GPIO1–10):** Jederzeit voll nutzbar (auch parallel zu WiFi/BLE).
  - **ADC2 (GPIO11–20):** Nicht nutzbar, solange WiFi oder BLE aktiv ist (Hardware-Konflikt im Chip).
- **USB-C-Buchsen (2× vorhanden):**
  - **Linke Buchse ("USB"):** Natives USB-Serial-JTAG / USB-OTG direkt über **GPIO19 (D−, Pin 25) / GPIO20 (D+, Pin 26)**.
  - **Rechte Buchse ("UART"):** Läuft über den On-Board **CH343P**-Chip auf **GPIO43 (U0TXD, Pin 43) / GPIO44 (U0RXD, Pin 42)**.
- **JTAG-Pins (GPIO39–42, Pin 36–39):** Als normale Digital-GPIOs frei nutzbar (sofern kein externes JTAG-Debugging über Pins genutzt wird).
- **RGB-LED (WS2812 an GPIO48, Pin 29):** Ab Werk aktiv verbunden und per `Adafruit_NeoPixel` / `FastLED` (1 Pixel) als Status-LED nutzbar. Daher GPIO48 nicht für empfindliche externe Eingänge (wie 1-Wire) verwenden.
