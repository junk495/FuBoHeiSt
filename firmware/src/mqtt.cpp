#include <Arduino.h>

#include <PubSubClient.h>
#include <WiFi.h>

#include <cstdlib>  // strtof, strtol
#include <cstring>  // strstr, strcmp, memcpy

#include "config.h"
#include "ds18b20_config.h"
#include "mqtt.h"

namespace Mqtt {

namespace {

// --- Publish-Client (unser Broker: Telemetrie) ---
WiFiClient pubWifiClient;
PubSubClient pubClient(pubWifiClient);

// --- Subscribe-Client (externer Sensor-Broker: Außentemperatur) ---
WiFiClient subWifiClient;
PubSubClient subClient(subWifiClient);

uint32_t lastPublishMs = 0;
uint32_t lastPubConnectMs = 0;
uint32_t lastSubConnectMs = 0;

// Außentemperatur-Cache (letzter gültiger Wert vom Sensor-Broker).
float    aussenTemp = 0.0f;
bool     aussenValid = false;
uint32_t aussenLastUpdateMs = 0;

// Räume → MQTT-Key → Raumfühler-Index (Config::ROOMS).
// Benennung (verbindlich): <kontext>_<eigenschaft>, Eigenschaft zuletzt.
struct Raum {
    const char* key;
    int roomIndex;
};

const Raum RAEUME[] = {
    {"wkue", 0},  // Wohnküche    → WohnKue
    {"gang", 5},  // Vorraum      → Gang
    {"sz1",  1},  // Schlafzimmer → Schlafz
    {"sz2",  2},  // Kinderzimmer → KiZi
    {"bad",  3},  // Bad          → Bad
    {"wc",   4},  // WC           → WC
};
constexpr int RAUM_COUNT = sizeof(RAEUME) / sizeof(RAEUME[0]);

// Heizkreise → MQTT-Key → DS18B20-Vorlaufsensor + Ventil-Kanal.
struct Kreis {
    const char* key;
    const char* dsName;   // DS18B20-Name (Vorlauf); nullptr = kein Sensor
    int valveChannel;     // ValveController-Kanal (0..7)
};

const Kreis KREISE[] = {
    {"wz1",  nullptr,    0},  // WZ1 (Wohnzimmer)
    {"wz2",  nullptr,    1},  // WZ2 (Wohnzimmer)
    {"kue",  nullptr,    2},  // KÜ  (Küche)
    {"gang", "Gang_Vor", 3},  // Gang (Vorraum)
    {"bad",  "Bad_Vor",  4},  // Bad
    {"sz1",  "SlZi_Vor", 6},  // SZ1 (Schlafzimmer)
    {"sz2",  "KiZi_Vor", 5},  // SZ2 (Kinderzimmer)
};
constexpr int KREIS_COUNT = sizeof(KREISE) / sizeof(KREISE[0]);

// Extrahiert Wert ("v") und Status ("s") aus dem Sensor-Payload
// {"ts":…,"v":…,"s":…}. Nur s == 0 (gültig) und plausible Temperatur werden übernommen.
//   s: 0 = gültig, 1 = unsicher, 2 = fehlerhaft.
bool parseAussen(const char* payload, float& out) {
    const char* pv = strstr(payload, "\"v\":");
    if (!pv) return false;
    const float v = strtof(pv + 4, nullptr);

    int s = 1;  // Standard: nicht übernehmen
    const char* ps = strstr(payload, "\"s\":");
    if (ps) s = static_cast<int>(strtol(ps + 4, nullptr, 10));
    if (s != 0) return false;

    if (v <= -60.0f || v >= 60.0f) return false;
    out = v;
    return true;
}

void onAussenMessage(char* topic, byte* payload, unsigned int length) {
    char buf[128];
    const unsigned int n = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
    memcpy(buf, payload, n);
    buf[n] = '\0';

    float v;
    if (parseAussen(buf, v)) {
        aussenTemp = v;
        aussenValid = true;
        aussenLastUpdateMs = millis();
        Serial.printf("Außentemperatur: %.1f °C\n", v);
    } else {
        Serial.println(F("Außentemperatur: Payload ungültig (verworfen)"));
    }
}

bool aussenFresh() {
    return aussenValid && (millis() - aussenLastUpdateMs < Config::AUSSEN_TIMEOUT_MS);
}

// DS18B20-Wert per Sensor-Name anhängen (1 Dezimalstelle) oder "null".
void appendDsTemp(String& s, const char* name,
                  const float dsTemps[], const bool dsValid[], int dsCount) {
    if (name) {
        for (int i = 0; i < dsCount; i++) {
            if (strcmp(Ds18b20Config::SENSORS[i].name, name) == 0) {
                if (dsValid[i]) s += String(dsTemps[i], 1);
                else s += "null";
                return;
            }
        }
    }
    s += "null";
}

// Baut den flachen JSON-Snapshot (vollständiges Schema; fehlend = null).
String buildJson(uint32_t ts,
                 const float roomTemps[], const bool roomValid[], int roomCount,
                 const float dsTemps[], const bool dsValid[], int dsCount,
                 const int posPct[], const bool posValid[], int posCount) {
    String s;
    s.reserve(700);

    s += "{\"ts\":";
    s += ts;

    s += ",\"aussen_t\":";
    if (aussenFresh()) s += String(aussenTemp, 1);
    else s += "null";

    // gemeinsame Vorlauf-/Rücklauftemperatur
    s += ",\"kreis_vl_t\":";
    appendDsTemp(s, "Vorlauf", dsTemps, dsValid, dsCount);
    s += ",\"kreis_rl_t\":null";  // noch kein Sensor

    // je Heizkreis: Vorlauf, Rücklauf, Ventilstellung
    for (int i = 0; i < KREIS_COUNT; i++) {
        s += ",\"kreis_";
        s += KREISE[i].key;
        s += "_vl_t\":";
        appendDsTemp(s, KREISE[i].dsName, dsTemps, dsValid, dsCount);

        s += ",\"kreis_";
        s += KREISE[i].key;
        s += "_rl_t\":null";  // noch kein Sensor

        s += ",\"kreis_";
        s += KREISE[i].key;
        s += "_pos\":";
        const int ch = KREISE[i].valveChannel;
        if (ch >= 0 && ch < posCount && posValid[ch]) {
            s += posPct[ch];
        } else {
            s += "null";
        }
    }

    // je Raum
    for (int i = 0; i < RAUM_COUNT; i++) {
        s += ",\"raum_";
        s += RAEUME[i].key;
        s += "_t\":";
        const int r = RAEUME[i].roomIndex;
        if (r >= 0 && r < roomCount && roomValid[r]) {
            s += String(roomTemps[r], 1);
        } else {
            s += "null";
        }
    }

    s += "}";
    return s;
}

// Eindeutige Client-ID (MAC-basiert) mit Präfix.
String clientId(const char* prefix) {
    String id = prefix;
    id += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
    return id;
}

}  // namespace

bool begin() {
    pubClient.setServer(Config::MQTT_BROKER_HOST, Config::MQTT_BROKER_PORT);
    pubClient.setSocketTimeout(2);
    pubClient.setKeepAlive(120);
    lastPublishMs = 0;
    lastPubConnectMs = 0;

    subClient.setServer(Config::AUSSEN_BROKER_HOST, Config::AUSSEN_BROKER_PORT);
    subClient.setSocketTimeout(2);
    subClient.setKeepAlive(120);
    subClient.setCallback(onAussenMessage);
    lastSubConnectMs = 0;
    return true;
}

bool connected() {
    return pubClient.connected();
}

bool tick(time_t ts,
          const float roomTemps[], const bool roomValid[], int roomCount,
          const float dsTemps[], const bool dsValid[], int dsCount,
          const int posPct[], const bool posValid[], int posCount) {
    if (WiFi.status() != WL_CONNECTED) return false;

    const uint32_t now = millis();
    bool published = false;

    // --- Publish-Client (Telemetrie) ---
    if (!pubClient.connected()) {
        if (now - lastPubConnectMs >= 10000) {  // Backoff 10 s
            lastPubConnectMs = now;
            String id = clientId("FuBoHeiSt-");
            pubClient.connect(id.c_str());
        }
    } else {
        pubClient.loop();
        if (now - lastPublishMs >= Config::MQTT_PUBLISH_INTERVAL_MS) {
            if (ts >= 1600000000LL) {
                const String payload = buildJson(static_cast<uint32_t>(ts),
                                                 roomTemps, roomValid, roomCount,
                                                 dsTemps, dsValid, dsCount,
                                                 posPct, posValid, posCount);
                published = pubClient.publish(Config::MQTT_TOPIC, payload.c_str());
                Serial.print("MQTT → ");
                Serial.println(payload);
            }
            lastPublishMs = now;
        }
    }

    // --- Subscribe-Client (Außentemperatur) ---
    if (!subClient.connected()) {
        if (now - lastSubConnectMs >= 10000) {  // Backoff 10 s
            lastSubConnectMs = now;
            String id = clientId("FuBoHeiSt-in-");
            if (subClient.connect(id.c_str())) {
                subClient.subscribe(Config::AUSSEN_TOPIC);
            }
        }
    } else {
        subClient.loop();
    }

    return published;
}

}  // namespace Mqtt
