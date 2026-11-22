#include <Arduino.h>

#include "thingspeak.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "secrets.h"

namespace ThingSpeak {

namespace {

// Führt den GET-Request auf die fertige Update-URL aus. Rückgabe: true bei HTTP 200.
bool doGet(const String& url) {
    WiFiClientSecure client;
    client.setInsecure();  // Dev-Kanäle; Zertifikatsprüfung hier bewusst aus
    HTTPClient http;
    http.begin(client, url);
    const int code = http.GET();
    const String payload = http.getString();
    http.end();

    Serial.printf("ThingSpeak: HTTP %d → %s\n", code, payload.c_str());
    return code == 200;
}

}  // namespace

bool publishRoomTemps(const float temps[], const bool valid[], int count) {
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
    if (!any) {
        Serial.println(F("ThingSpeak: keine gültigen Werte → nichts gesendet"));
        return false;
    }
    return doGet(url);
}

bool publishDs18b20(const Field fields[], int count) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("ThingSpeak: kein WLAN"));
        return false;
    }

    String url = "https://api.thingspeak.com/update?api_key=";
    url += THINGSPEAK_DS18B20_API_KEY;

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (!fields[i].valid || fields[i].field <= 0) continue;
        url += "&field";
        url += fields[i].field;
        url += "=";
        url += String(fields[i].value, 2);  // 2 Nachkommastellen
        any = true;
    }
    if (!any) {
        Serial.println(F("ThingSpeak: keine gültigen Werte → nichts gesendet"));
        return false;
    }
    return doGet(url);
}

}  // namespace ThingSpeak
