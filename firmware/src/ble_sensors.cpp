#include <Arduino.h>

#include "ble_sensors.h"

#include <NimBLEDevice.h>

#include "config.h"

namespace {

const NimBLEUUID kBthomeService(static_cast<uint16_t>(0xFCD2));

float gTemp[Config::ROOM_COUNT];
bool  gValid[Config::ROOM_COUNT];

NimBLEScan* pScan = nullptr;

int16_t readInt16Le(const uint8_t* p) {
    return static_cast<int16_t>(static_cast<uint16_t>(p[0]) |
                                (static_cast<uint16_t>(p[1]) << 8));
}

// MAC-Vergleich ohne Beachtung der Groß-/Kleinschreibung
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

int roomIndexByMac(const std::string& mac) {
    for (int i = 0; i < Config::ROOM_COUNT; i++) {
        if (macEquals(mac, Config::ROOMS[i].mac)) return i;
    }
    return -1;
}

// Länge der BTHome-v2-Daten je Objekt-ID (nur zum Durchlaufen nötig).
uint8_t bthomeObjectLength(uint8_t id) {
    switch (id) {
        case 0x00: return 1;  // Packet-ID
        case 0x01: return 1;  // Batterie %
        case 0x02: return 2;  // Temperatur 0,01 °C
        case 0x03: return 2;  // Luftfeuchte 0,01 %
        case 0x0C: return 2;  // Spannung 0,001 V
        default:   return 0;  // unbekannt → Abbruch
    }
}

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        std::string raw = device->getServiceData(kBthomeService);
        if (raw.size() < 2) return;
        const auto* d = reinterpret_cast<const uint8_t*>(raw.data());

        // d[0] = BTHome-Device-Info; danach Messwert-Objekte durchlaufen
        bool haveTemp = false;
        float temp = 0.0f;
        size_t pos = 1;
        while (pos < raw.size()) {
            const uint8_t id = d[pos++];
            const uint8_t len = bthomeObjectLength(id);
            if (len == 0 || pos + len > raw.size()) return;
            if (id == 0x02) {
                temp = readInt16Le(d + pos) / 100.0f;
                haveTemp = true;
            }
            pos += len;
        }
        if (!haveTemp) return;

        const int idx = roomIndexByMac(device->getAddress().toString());
        if (idx >= 0) {
            gTemp[idx] = temp;
            gValid[idx] = true;
        }
    }
};

}  // namespace

namespace BleSensors {

void begin() {
    for (int i = 0; i < Config::ROOM_COUNT; i++) gValid[i] = false;

    NimBLEDevice::init("FuBoHeiSt");
    pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new ScanCallbacks(), true);
    pScan->setActiveScan(false);   // passiv
    pScan->setInterval(100);       // ms
    pScan->setWindow(100);         // ms
    pScan->setDuplicateFilter(0);  // Duplikate sichtbar
}

void scan() {
    if (pScan->isScanning()) return;
    pScan->start(Config::SCAN_WINDOW_MS);
    while (pScan->isScanning()) delay(20);
}

bool hasTemp(int roomIndex) {
    return roomIndex >= 0 && roomIndex < Config::ROOM_COUNT && gValid[roomIndex];
}

float temp(int roomIndex) {
    return gTemp[roomIndex];
}

}  // namespace BleSensors
