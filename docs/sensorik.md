# Sensorik – Raumfühler (BLE-Temperatur)

> Offener Punkt aus `README.md` §6: **BLE-Sensor-Modell festlegen**.
> Dieses Dokument vergleicht Kandidaten, legt das BLE-Protokoll fest und beschreibt die
> ESP32-S3-Anbindung (WiFi/BLE-Koexistenz) samt Fallback im `Supervisor`.

## 1. Anforderungen

- **6 Raumfühler** für 7 Zonen (Küche + Wohnzimmer teilen sich einen Fühler, siehe `README.md` §2).
- Messgröße: **Raumtemperatur** (Pflicht), Luftfeuchte (optional).
- **Batteriebetrieben**, wartungsarm (mehrjährige Laufzeit).
- **Offline-first:** kein Cloud-/App-Zwang, keine Konto-/Binding-Pflicht.
- Zone 1 (Bad) und Zone 2 (WC): **feuchtraumtauglich** (IP-Schutz).

## 2. Kandidaten-Gegenüberstellung

| Kandidat | BTHome v2 | Feuchtraum (IP) | Bemerkung |
|---|---|---|---|
| Xiaomi LYWSD03MMC | ✅ über pvvx-Custom-FW (Flash/OTA nötig) | nein | sehr günstig; **Hardware-Versions-Risiko** (BLE-Chip, siehe `README.md` §6) |
| Shelly BLU H&T | ✅ nativ | **IP54** | für Bad/WC geeignet; kein Flashen nötig |
| Qingping CGDK2 | ❌ nicht nativ (zu verifizieren) | zu verifizieren | App-/Ökosystem-Abhängigkeit |
| SwitchBot Meter | ❌ nicht nativ (proprietäres Format) | zu verifizieren | eigenes BLE-Advertisement |

**Empfehlung:** **Shelly BLU H&T** als Standard (nativ BTHome v2, IP54, kein Flashen);
**LYWSD03MMC + pvvx-FW** als günstige Alternative, sofern das Versions-Risiko akzeptiert wird.

## 3. Protokoll-Entscheidung: BTHome v2

- Service-UUID **`0xFCD2`**, **unverschlüsselt**.
- **Passives BLE-Advertisement** (Broadcast) – **kein Verbindungsaufbau**, kein Pairing.
- Temperatur/Feuchte/Batterie stehen direkt im Advertisement → der ESP32 muss nur „mithören".
- BTHome v2 ist offen dokumentiert und wird von Home Assistant nativ verstanden.

## 4. ESP32-S3-Koexistenz (WiFi + BLE)

- Library: **NimBLE-Arduino** (geringer RAM-/Flash-Bedarf als klassisches Bluedroid).
- **Intervall-Scanning:** alle **60 s** für **10 s** passiv scannen, danach BLE **pausieren**
  → störungsfreier MQTT/WiFi-Betrieb im restlichen Zeitfenster.
- Ein 10-s-Fenster genügt für den Broadcast-Intervall der Fühler (typisch 1–10 s).
- Zuordnung Zone ↔ Fühler über die **BLE-MAC-Adresse** (persistent im NVS).

## 5. Watchdog / Fallback (`Supervisor`)

- **Timeout:** 30 Minuten ohne gültiges BLE-Paket einer Zone.
- **Fallback:** Regelung dieser Zone auf **Außentemperatur-Vorsteuerung**
  (ohne Ist-Raumtemp.-Rückführung) + **MQTT-Statusmeldung** (`fbh/zone/<id>/status = error`).
- Sobald wieder gültige Pakete eintreffen → automatische Rückkehr in den Regelbetrieb.

## 6. Offen / TODO

- [ ] Finales Fühlermodell beschaffen und testen (Empfehlung: Shelly BLU H&T für Bad/WC, Rest nach Budget).
- [ ] IP-/Batterie-Angaben der Kandidaten (v. a. Qingping, SwitchBot) beim Kauf final verifizieren.
- [ ] pvvx-Firmware-Version des LYWSD03MMC gegen BTHome-v2-Advertisement testen.
