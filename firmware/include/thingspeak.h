#pragma once

// ThingSpeak-Anbindung.
namespace ThingSpeak {

// Sendet die Temperaturen als field<N>=<wert> (nur gültige Werte).
// Rückgabe: true bei HTTP 200.
bool publishTemps(const float temps[], const bool valid[], int count);

}  // namespace ThingSpeak
