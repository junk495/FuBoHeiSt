# Motor-Bringup (Stufe 1, Referenz)

Das ist der ursprüngliche Bringup-/Diagnose-Code aus `firmware/src/main.cpp`
(serielles Kommando-Interface zum Testen von DRV8833, INA219, Ripple-Zählung,
DS18B20). Er wurde durch die Sensor-Logging-Firmware ersetzt und liegt hier nur
noch als Referenz.

Zum erneuten Bauen die Datei `main.cpp` zurück nach `firmware/src/main.cpp`
kopieren. Die Motor-Module (`MotorDriver`, `CurrentSense`, `RippleCounter`) und
`pins.h` liegen weiterhin in `firmware/src` bzw. `firmware/include`.
