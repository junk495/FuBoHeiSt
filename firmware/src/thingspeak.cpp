#include <Arduino.h>

#include "thingspeak.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "secrets.h"

namespace ThingSpeak {

bool publishTemps(const float temps[], const bool valid[], int count,
                  const Field extra[], int extraCount) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("ThingSpeak: kein WLAN"));
        return false;
    }

    String url = "https://api.thingspeak.com/update?api_key=";
    url += THINGSPEAK_API_KEY;

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (!valid[i]) continue;
        url += "&field";
        url += Config::ROOMS[i].field;
        url += "=";
        url += String(temps[i], 2);  // 2 Nachkommastellen
        any = true;
    }

    // Zusätzliche Felder (z. B. DS18B20-Sensoren)
    for (int i = 0; i < extraCount; i++) {
        if (!extra[i].valid || extra[i].field <= 0) continue;
        url += "&field";
        url += extra[i].field;
        url += "=";
        url += String(extra[i].value, 2);  // 2 Nachkommastellen
        any = true;
    }
    if (!any) {
        Serial.println(F("ThingSpeak: keine gültigen Werte → nichts gesendet"));
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();  // Dev-Kanal; Zertifikatsprüfung hier bewusst aus
    HTTPClient http;
    http.begin(client, url);
    const int code = http.GET();
    const String payload = http.getString();
    http.end();

    Serial.printf("ThingSpeak: HTTP %d → %s\n", code, payload.c_str());
    return code == 200;
}

}  // namespace ThingSpeak
