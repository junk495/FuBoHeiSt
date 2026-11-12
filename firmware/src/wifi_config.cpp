#include <Arduino.h>

#include "wifi_config.h"

#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {

constexpr char kNvsNamespace[] = "fuboheist";
constexpr char kApSsid[] = "FuBoHeiSt-Setup";

Preferences prefs;
WebServer server(80);
DNSServer dns;

const char kPageHtml[] = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FuBoHeiSt – WLAN</title>
</head>
<body style="font-family:sans-serif;padding:1.5em">
  <h1>FuBoHeiSt – WLAN einrichten</h1>
  <form method="post" action="/save">
    <label>SSID:<br><input name="ssid" required></label><br><br>
    <label>Passwort:<br><input name="pass" type="password"></label><br><br>
    <button type="submit">Speichern</button>
  </form>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html; charset=utf-8", kPageHtml);
}

void handleSave() {
    const String ssid = server.arg("ssid");
    const String pass = server.arg("pass");
    if (ssid.length() == 0) {
        server.send(400, "text/plain; charset=utf-8", "SSID fehlt");
        return;
    }
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    server.send(200, "text/plain; charset=utf-8",
                "Gespeichert. Der ESP32 startet neu und verbindet sich …");
    delay(500);
    ESP.restart();
}

bool tryConnect() {
    const String ssid = prefs.getString("ssid", "");
    if (ssid.length() == 0) {
        Serial.println(F("Keine WLAN-Daten gespeichert → Captive-Portal"));
        return false;
    }
    const String pass = prefs.getString("pass", "");

    Serial.printf("Verbinde mit \"%s\" …\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Verbunden, IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.println(F("Verbindung fehlgeschlagen → Captive-Portal"));
    WiFi.disconnect(true);
    return false;
}

void runPortal() {
    Serial.println(F("Starte Hotspot \"FuBoHeiSt-Setup\" …"));
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid);
    dns.start(53, "*", WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleRoot);
    server.begin();

    Serial.printf("Mit WLAN \"%s\" verbinden (IP %s) und Seite öffnen.\n",
                  kApSsid, WiFi.softAPIP().toString().c_str());

    while (true) {  // läuft bis ESP.restart() nach dem Speichern
        dns.processNextRequest();
        server.handleClient();
        delay(10);
    }
}

}  // namespace

namespace WifiConfig {

void begin() {
    prefs.begin(kNvsNamespace, false);  // read-write
    if (tryConnect()) return;
    runPortal();
}

}  // namespace WifiConfig
