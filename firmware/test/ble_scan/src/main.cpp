// FuBoHeiSt – BLE-Empfangstest (Xiaomi LYWSD03MMC mit Custom-Firmware)
//
// Zweck: Prüfen, ob der ESP32-S3 die BLE-Advertisements der Fühler empfängt und
// Temperatur, Luftfeuchte und Batterie korrekt ausliest. Passiver Scan, kein
// Verbindungsaufbau (offline-first).
//
// Unterstützte Formate:
//   1) ATC "Custom" (Standard der ATC-/pvvx-Firmware): Service-Daten UUID 0x181A
//      Layout (13 Byte, Quelle: ESPHome atc_mithermometer):
//        [0..5]   MAC (Anzeige-Reihenfolge)
//        [6..7]   Temperatur int16 LE, 0,1 °C
//        [8]      Luftfeuchte %
//        [9]      Batterie %
//        [10..11] Batteriespannung uint16 LE, mV
//        [12]     Frame-Zähler
//   2) BTHome v2 (Zielformat laut docs/sensorik.md): Service-Daten UUID 0xFCD2
// Unbekannte, aber interessante Geräte werden als Rohdaten (Hex) ausgegeben.

#include <Arduino.h>
#include <NimBLEDevice.h>

namespace {

constexpr uint32_t kScanWindowMs = 30000;  // Scandauer je Durchgang (30 s)
constexpr uint16_t kScanInterval = 100;    // Scan-Intervall (ms)
constexpr uint16_t kScanWindow   = 100;    // Scan-Fenster (ms)

const NimBLEUUID kAtcService(static_cast<uint16_t>(0x181A));     // ATC custom (Service Data)
const NimBLEUUID kBthomeService(static_cast<uint16_t>(0xFCD2));  // BTHome v2 (Service Data)

struct KnownSensor {
    const char* zone;
    const char* name;  // BLE-Advertising-Name
    const char* mac;
};

// Bekannte Fühler aus sensors/sensors.txt (nur zur Anzeige/Zuordnung, kein Filter).
// BLE-Namen von Zone 3 (KiZi/Kinderzimmer) und 4 (Bad) sind angenommen –
// entscheidend ist die MAC, die Zuordnung klappt auch ohne passenden Namen.
const KnownSensor kKnownSensors[] = {
    {"WohnKue", "WohnKue", "A4:C1:38:C2:92:A7"},
    {"Schlafz", "Schlafz", "A4:C1:38:51:E4:5B"},
    {"KiZi",    "KiZi",    "A4:C1:38:E0:BA:0F"},
    {"Bad",     "Bad",     "A4:C1:38:36:FB:F1"},
};

}  // namespace

NimBLEScan* pScan = nullptr;

// 16-Bit-Wert (Little-Endian) sicher dekodieren
int16_t readInt16Le(const uint8_t* p) {
    return static_cast<int16_t>(static_cast<uint16_t>(p[0]) |
                                (static_cast<uint16_t>(p[1]) << 8));
}

uint16_t readUint16Le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

// MAC-Vergleich ohne Beachtung der Groß-/Kleinschreibung
// (NimBLE liefert "a4:c1:…", unsere Liste nutzt "A4:C1:…")
bool macEquals(const std::string& a, const char* b) {
    size_t i = 0;
    for (; b[i] != '\0'; i++) {
        if (i >= a.size()) return false;
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb + 32);
        if (ca != cb) return false;
    }
    return a.size() == i;
}

// Zone zur MAC ermitteln (nur Anzeige)
const char* zoneByMac(const std::string& mac) {
    for (const auto& s : kKnownSensors) {
        if (macEquals(mac, s.mac)) return s.zone;
    }
    return nullptr;
}

void printHex(const std::string& data) {
    const auto* d = reinterpret_cast<const uint8_t*>(data.data());
    for (size_t i = 0; i < data.size(); i++) {
        if (d[i] < 0x10) Serial.print('0');
        Serial.print(d[i], HEX);
        Serial.print(' ');
    }
}

// ATC-"Custom"-Service-Daten (UUID 0x181A) auswerten
bool parseAtc(const NimBLEAdvertisedDevice* dev) {
    std::string raw = dev->getServiceData(kAtcService);
    if (raw.size() < 13) return false;
    const auto* d = reinterpret_cast<const uint8_t*>(raw.data());

    const float temp     = readInt16Le(d + 6) / 10.0f;
    const uint8_t hum    = d[8];
    const uint8_t batP   = d[9];
    const uint16_t batMv = readUint16Le(d + 10);
    const uint8_t frame  = d[12];

    const std::string mac = dev->getAddress().toString();
    const char* zone = zoneByMac(mac);

    Serial.printf("ATC   %-8s %-17s %4d dBm | %5.1f °C | %2u %%rF | Bat %3u %% (%u mV) | f=%u\n",
                  zone ? zone : "-", mac.c_str(), static_cast<int>(dev->getRSSI()),
                  temp, hum, batP, batMv, frame);
    return true;
}

// Länge der BTHome-v2-Daten je Objekt-ID (Quelle: https://bthome.io/format/)
uint8_t bthomeObjectLength(uint8_t id) {
    switch (id) {
        case 0x00: return 1;  // Packet-ID
        case 0x01: return 1;  // Batterie %
        case 0x02: return 2;  // Temperatur 0,01 °C
        case 0x03: return 2;  // Luftfeuchte 0,01 %
        case 0x04: return 3;  // Luftdruck
        case 0x05: return 3;  // Helligkeit
        case 0x06: return 2;  // Masse kg
        case 0x07: return 2;  // Masse lb
        case 0x08: return 2;  // Taupunkt
        case 0x09: return 1;  // Zähler (uint8)
        case 0x0A: return 3;  // Energie
        case 0x0B: return 3;  // Leistung
        case 0x0C: return 2;  // Spannung 0,001 V
        case 0x0D: return 2;  // PM2.5
        case 0x0E: return 2;  // PM10
        case 0x0F: return 1;  // Boolean
        case 0x10: return 1;  // Boolean
        case 0x11: return 1;  // Boolean
        default:   return 0;  // unbekannt → Abbruch
    }
}

// BTHome-v2-Service-Daten (UUID 0xFCD2) auswerten
bool parseBthome(const NimBLEAdvertisedDevice* dev) {
    std::string raw = dev->getServiceData(kBthomeService);
    if (raw.size() < 2) return false;
    const auto* d = reinterpret_cast<const uint8_t*>(raw.data());
    // d[0] = BTHome-Device-Info; danach Messwert-Objekte
    bool haveTemp = false, haveHum = false, haveBat = false, haveVolt = false;
    float temp = 0.0f;
    uint16_t hum = 0;
    uint8_t bat = 0;
    float volt = 0.0f;

    size_t pos = 1;
    while (pos < raw.size()) {
        const uint8_t id = d[pos++];
        const uint8_t len = bthomeObjectLength(id);
        if (len == 0 || pos + len > raw.size()) break;  // unbekannt oder unvollständig

        switch (id) {
            case 0x01: bat = d[pos]; haveBat = true; break;                                    // Batterie %
            case 0x02: temp = readInt16Le(d + pos) / 100.0f; haveTemp = true; break;           // Temp 0,01 °C
            case 0x03: hum = readUint16Le(d + pos) / 100; haveHum = true; break;               // Feuchte 0,01 %
            case 0x0C: volt = readUint16Le(d + pos) / 1000.0f; haveVolt = true; break;         // Spannung 0,001 V
            default: break;  // übrige Objekte überspringen
        }
        pos += len;
    }
    if (!(haveTemp || haveHum || haveBat || haveVolt)) return false;

    const std::string mac = dev->getAddress().toString();
    const char* zone = zoneByMac(mac);

    Serial.printf("BTHome %-8s %-17s %4d dBm |", zone ? zone : "-", mac.c_str(),
                  static_cast<int>(dev->getRSSI()));
    if (haveTemp) Serial.printf(" %5.1f °C", temp);
    if (haveHum)  Serial.printf(" %2u %%rF", hum);
    if (haveBat)  Serial.printf(" Bat %3u %%", bat);
    if (haveVolt) Serial.printf(" %4.2f V", volt);
    Serial.println();
    return true;
}

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        // Nur die 4 bekannten Fühler anzeigen – alles andere ignorieren.
        const std::string mac = device->getAddress().toString();
        const char* zone = zoneByMac(mac);
        if (zone == nullptr) return;

        if (parseAtc(device) || parseBthome(device)) return;

        // Bekannter Fühler, aber (noch) nicht geparst → Rohdaten (z. B. MiBeacon 0xFE95)
        const std::string name = device->getName();
        const std::string man  = device->getManufacturerData();
        const uint8_t svcCount = device->getServiceDataCount();

        Serial.printf("???   %-8s %-17s %4d dBm | name=\"%s\" | srv=%u man=%u\n",
                      zone, mac.c_str(), static_cast<int>(device->getRSSI()),
                      name.c_str(), static_cast<unsigned int>(svcCount),
                      static_cast<unsigned int>(man.size()));
        if (!man.empty()) {
            Serial.print("      Manufacturer: ");
            printHex(man);
            Serial.println();
        }
        for (uint8_t i = 0; i < svcCount; i++) {
            Serial.printf("      ServiceData[%u] %s: ", i,
                          device->getServiceDataUUID(i).toString().c_str());
            printHex(device->getServiceData(i));
            Serial.println();
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);  // USB-CDC verbinden lassen

    Serial.println();
    Serial.println(F("FuBoHeiSt – BLE-Empfangstest (LYWSD03MMC)"));
    Serial.printf("Bekannte Fühler: %s (Name \"%s\", %s), %s (Name \"%s\", %s)\n",
                  kKnownSensors[0].zone, kKnownSensors[0].name, kKnownSensors[0].mac,
                  kKnownSensors[1].zone, kKnownSensors[1].name, kKnownSensors[1].mac);
    Serial.printf("Scan: passiv, Fenster %lu s, Duplikate sichtbar\n\n",
                  static_cast<unsigned long>(kScanWindowMs / 1000));

    NimBLEDevice::init("FuBoHeiSt-Scan");
    pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new ScanCallbacks(), true);
    pScan->setActiveScan(false);          // passiv – nur mithören
    pScan->setInterval(kScanInterval);
    pScan->setWindow(kScanWindow);
    pScan->setDuplicateFilter(0);         // Duplikate sichtbar machen
}

void loop() {
    if (!pScan->isScanning()) {
        Serial.println(F("--- Scan startet ---"));
        pScan->start(kScanWindowMs);
    }
    delay(50);
}

