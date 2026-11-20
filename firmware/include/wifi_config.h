#pragma once

// WLAN-Verbindung mit Captive-Portal (nicht-blockierend).
namespace WifiConfig {

// Bereitet die Verbindung vor: mit gespeicherten Zugangsdaten verbinden oder –
// wenn keine vorhanden sind – den Hotspot "FuBoHeiSt-Setup" starten.
// Blockiert nicht; der Fortschritt läuft in tick() weiter.
void begin();

// Nicht-blockierender Zustandsautomat: Verbindungsaufbau, automatischer
// Reconnect (mit Backoff), Portal-Server und DNS-Fallback.
// Muss aus loop() regelmäßig aufgerufen werden.
void tick();

// true, wenn die Station mit dem WLAN verbunden ist.
bool connected();

// Gibt den WLAN-Status (SSID, Zustand, IP, RSSI) auf der seriellen Konsole aus.
void printStatus();

// Löscht die gespeicherten Zugangsdaten aus dem NVS und startet neu
// (→ Captive-Portal zur Neukonfiguration).
void resetCredentials();

}  // namespace WifiConfig
