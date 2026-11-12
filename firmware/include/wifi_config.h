#pragma once

// WLAN-Verbindung mit Captive-Portal.
namespace WifiConfig {

// Verbindet mit den im NVS gespeicherten Zugangsdaten. Sind keine vorhanden
// (oder die Verbindung schlägt fehl), startet ein Hotspot "FuBoHeiSt-Setup"
// mit Webseite zur Eingabe. Blockiert, bis WLAN verbunden ist.
void begin();

}  // namespace WifiConfig
