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

// Gespeicherte Zugangsdaten (Quelle: NVS, überleben normales Flashen).
String sSavedSsid;
String sSavedPass;

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
  <p style="color:#666;font-size:0.9em">
    Die Zugangsdaten werden dauerhaft im Flash (NVS) gespeichert und bleiben
    bei einem normalen Firmware-Upload erhalten.
  </p>
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
    sSavedSsid = ssid;
    sSavedPass = pass;
    server.send(200, "text/plain; charset=utf-8",
                "Gespeichert. Der ESP32 startet neu und verbindet sich …");
    delay(500);
    ESP.restart();
}

// Robustheits-/Retry-Parameter.
constexpr uint32_t kConnectTimeoutMs = 20000;   // Timeout erster Verbindungsversuch
constexpr uint32_t kReconnectBaseMs = 5000;     // erster Reconnect-Abstand
constexpr uint32_t kReconnectMaxMs = 60000;     // max. Reconnect-Abstand (Backoff)
constexpr uint8_t  kMaxConnectFails = 8;        // danach → Captive-Portal

enum class State { Connecting, Connected, Reconnecting, Portal };

State    sState = State::Connecting;
uint32_t sStateStart = 0;
uint32_t sLastDot = 0;
uint32_t sReconnectDelayMs = kReconnectBaseMs;
uint8_t  sFailCount = 0;

// Meldet WLAN-Ereignisse auf der seriellen Konsole (nur Diagnose).
void onWifiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println(F("[WLAN] STA verbunden – warte auf IP."));
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println(F("[WLAN] STA getrennt."));
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("[WLAN] IP: %s\n", WiFi.localIP().toString().c_str());
            break;
        default:
            break;
    }
}

void printConnected() {
    Serial.printf("Verbunden, IP: %s, RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

void startReconnect() {
    sState = State::Reconnecting;
    sStateStart = millis();
    sReconnectDelayMs = kReconnectBaseMs;
    Serial.println(F("→ Reconnect-Modus."));
}

void startPortal() {
    Serial.println(F("Starte Hotspot \"FuBoHeiSt-Setup\" …"));
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid);
    dns.start(53, "*", WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleRoot);
    server.begin();

    Serial.printf("Mit WLAN \"%s\" verbinden (IP %s) und Seite öffnen.\n",
                  kApSsid, WiFi.softAPIP().toString().c_str());

    sState = State::Portal;
}

const char* stateName() {
    switch (sState) {
        case State::Connecting:   return "verbindet";
        case State::Connected:    return "verbunden";
        case State::Reconnecting: return "reconnect";
        case State::Portal:       return "portal";
        default:                  return "unbekannt";
    }
}

}  // namespace

namespace WifiConfig {

void begin() {
    prefs.begin(kNvsNamespace, false);  // read-write (Portal speichert später)

    sSavedSsid = prefs.getString("ssid", "");
    sSavedPass = prefs.getString("pass", "");

    // Robuste Verbindung: Modem-Sleep aus, Auto-Reconnect an, keine
    // Doppel-Ablage der Zugangsdaten durch die WiFi-Bibliothek (wir nutzen NVS).
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);

    WiFi.onEvent(onWifiEvent, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWifiEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onWifiEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    if (sSavedSsid.length() == 0) {
        Serial.println(F("Keine WLAN-Daten gespeichert → Captive-Portal"));
        startPortal();
        return;
    }

    Serial.printf("Verbinde mit \"%s\" …\n", sSavedSsid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(sSavedSsid.c_str(), sSavedPass.c_str());

    sState = State::Connecting;
    sStateStart = millis();
    sLastDot = millis();
}

void tick() {
    switch (sState) {
        case State::Connecting:
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println();
                printConnected();
                sState = State::Connected;
            } else if (millis() - sStateStart >= kConnectTimeoutMs) {
                Serial.println();
                Serial.println(F("Verbindung fehlgeschlagen."));
                sFailCount++;
                if (sFailCount >= kMaxConnectFails) {
                    Serial.println(F("Zu viele Fehlversuche → Captive-Portal."));
                    startPortal();
                } else {
                    startReconnect();
                }
            } else if (millis() - sLastDot >= 250) {
                Serial.print('.');
                sLastDot = millis();
            }
            break;

        case State::Connected:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println(F("WLAN-Verbindung verloren."));
                sFailCount = 0;
                startReconnect();
            }
            break;

        case State::Reconnecting:
            if (WiFi.status() == WL_CONNECTED) {
                printConnected();
                sState = State::Connected;
            } else if (millis() - sStateStart >= sReconnectDelayMs) {
                sFailCount++;
                Serial.printf("Reconnect-Versuch %d/%d …\n",
                              sFailCount, kMaxConnectFails);
                WiFi.disconnect(false);
                delay(10);
                WiFi.begin(sSavedSsid.c_str(), sSavedPass.c_str());
                sStateStart = millis();
                sReconnectDelayMs = sReconnectDelayMs * 2;
                if (sReconnectDelayMs > kReconnectMaxMs) {
                    sReconnectDelayMs = kReconnectMaxMs;
                }
                if (sFailCount >= kMaxConnectFails) {
                    Serial.println(F("Zu viele Reconnect-Versuche → Captive-Portal."));
                    startPortal();
                }
            }
            break;

        case State::Portal:
            dns.processNextRequest();
            server.handleClient();
            break;
    }
}

bool connected() {
    return sState == State::Connected && WiFi.status() == WL_CONNECTED;
}

void printStatus() {
    Serial.println(F("--- WLAN-Status ---"));
    if (sSavedSsid.length() == 0) {
        Serial.println(F("  Gespeicherte SSID: (keine)"));
    } else {
        Serial.printf("  Gespeicherte SSID: %s\n", sSavedSsid.c_str());
    }
    Serial.printf("  Zustand: %s\n", stateName());
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("  IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.printf("  WLAN-Status-Code: %d\n", static_cast<int>(WiFi.status()));
    }
}

void resetCredentials() {
    Serial.println(F("Lösche gespeicherte WLAN-Daten und starte neu …"));
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
    delay(200);
    ESP.restart();
}

}  // namespace WifiConfig
