#pragma once

// WLAN-Verbindung mit Captive-Portal (nicht-blockierend).
namespace WifiConfig {

// Bereitet die Verbindung vor: mit gespeicherten Zugangsdaten verbinden oder –
// wenn keine vorhanden sind – den Hotspot "FuBoHeiSt-Setup" starten.
// Blockiert nicht; der Fortschritt läuft in tick() weiter.
void begin();

// Nicht-blockierender Zustandsautomat: Verbindungsversuch, Portal-Server und
// DNS-Fallback. Muss aus loop() regelmäßig aufgerufen werden.
void tick();

// true, wenn die Station mit dem WLAN verbunden ist.
bool connected();

}  // namespace WifiConfig
