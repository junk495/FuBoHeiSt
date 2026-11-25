#include <Arduino.h>

#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "mqtt.h"

namespace Mqtt {

namespace {

WiFiClient wifiClient;
PubSubClient client(wifiClient);
uint32_t lastPublishMs = 0;
uint32_t lastConnectAttemptMs = 0;

// Zuordnung der 7 Heizkreise → MQTT-Key → Raumfühler-Index (Config::ROOMS).
// Quelle der Raumzuordnung: docs/heizung-system.md §3.
struct Kreis {
    const char* key;
    int roomIndex;  // Index in Config::ROOMS; -1 = (noch) kein Fühler
};

const Kreis KREISE[] = {
    {"wz1", 0},   // WZ1 (Wohnzimmer)   → WohnKue
    {"wz2", 0},   // WZ2 (Wohnzimmer)   → WohnKue
    {"kue", 0},   // KÜ  (Küche)        → WohnKue
    {"gang", 5},  // Gang (Vorraum)     → Gang
    {"bad", 3},   // Bad  (Bad + WC)    → Bad (WC-Fühler aktuell nicht abgebildet)
    {"sz2", 2},   // SZ2 (Kinderzimmer) → KiZi
    {"sz1", 1},   // SZ1 (Schlafzimmer) → Schlafz
};
constexpr int KREIS_COUNT = sizeof(KREISE) / sizeof(KREISE[0]);

// Baut den JSON-Snapshot (aktuell nur Raumtemperaturen).
String buildJson(uint32_t ts, const float temps[], const bool valid[], int count) {
    String s;
    s.reserve(320);
    s += "{\"ts\":";
    s += ts;
    s += ",\"kreise\":{";
    for (int i = 0; i < KREIS_COUNT; i++) {
        if (i) s += ',';
        s += '"';
        s += KREISE[i].key;
        s += "\":{\"t_raum\":";
        const int r = KREISE[i].roomIndex;
        if (r >= 0 && r < count && valid[r]) {
            s += String(temps[r], 1);  // immer 1 Dezimalstelle → "21.0" statt "21"
        } else {
            s += "null";               // keine Fantasiewerte (Regel 3)
        }
        s += '}';
    }
    s += "}}";
    return s;
}

}  // namespace

bool begin() {
    client.setServer(Config::MQTT_BROKER_HOST, Config::MQTT_BROKER_PORT);
    client.setSocketTimeout(2);   // kurzer Timeout
    client.setKeepAlive(120);     // > Publish-Intervall
    lastPublishMs = 0;
    lastConnectAttemptMs = 0;
    return true;
}

bool connected() {
    return client.connected();
}

bool tick(time_t ts, const float temps[], const bool valid[], int count) {
    if (WiFi.status() != WL_CONNECTED) return false;

    const uint32_t now = millis();

    // Verbindung aufbauen/halten – mit Backoff, damit ein nicht erreichbarer
    // Broker den Loop nicht dauerhaft blockiert.
    if (!client.connected()) {
        if (now - lastConnectAttemptMs < 10000) return false;  // Backoff 10 s
        lastConnectAttemptMs = now;

        String id = "FuBoHeiSt-";
        id += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
        client.connect(id.c_str());
        if (!client.connected()) return false;
    }
    client.loop();

    if (now - lastPublishMs < Config::MQTT_PUBLISH_INTERVAL_MS) return false;

    // Ohne gültige Zeit (NTP noch nicht synchronisiert) keinen ts senden.
    if (ts < 1600000000LL) return false;

    const String payload = buildJson(static_cast<uint32_t>(ts), temps, valid, count);
    const bool ok = client.publish(Config::MQTT_TOPIC, payload.c_str());
    Serial.print("MQTT → ");
    Serial.println(payload);
    lastPublishMs = now;
    return ok;
}

}  // namespace Mqtt