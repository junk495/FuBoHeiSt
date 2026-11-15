#pragma once

#include <cstdint>

// DRV8833-Wahrheitstabelle (xIN1/xIN2):
//   0/0 = Coast (fast decay), 0/1 = Reverse, 1/0 = Forward, 1/1 = Brake (slow decay)
enum class MotorState : uint8_t {
    Coast,
    Brake,
    Forward,
    Reverse,
};

class MotorDriver {
public:
    void begin();               // Pins konfigurieren, Treiber im Sleep
    void enable();              // nSLEEP = HIGH (Treiber aktiv)
    void disable();             // nSLEEP = LOW (Sleep)
    bool isEnabled() const;
    bool fault() const;         // true = nFAULT aktiv (LOW)

    void set(uint8_t channel, MotorState state);
    void coast(uint8_t channel);
    void brake(uint8_t channel);
    void forward(uint8_t channel);
    void reverse(uint8_t channel);

private:
    uint8_t in1(uint8_t channel) const;
    uint8_t in2(uint8_t channel) const;
};
