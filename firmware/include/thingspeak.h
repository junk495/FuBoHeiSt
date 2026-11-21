#pragma once

// ThingSpeak-Anbindung.
namespace ThingSpeak {

// Sendet die Raumtemperaturen als field<N>=<wert> (nur gültige Werte) und
// optional die Vorlauftemperatur auf Config::SUPPLY_FIELD.
// Rückgabe: true bei HTTP 200.
bool publishTemps(const float temps[], const bool valid[], int count,
                  float supplyTemp, bool supplyValid);

}  // namespace ThingSpeak
