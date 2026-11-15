#include "MotorDriver.h"
#include "pins.h"
#include <Arduino.h>

void MotorDriver::begin() {
    pinMode(Pin::MOTOR_nSLEEP, OUTPUT);
    digitalWrite(Pin::MOTOR_nSLEEP, LOW);   // beim Boot sicher deaktiviert

    pinMode(Pin::MOTOR_nFAULT, INPUT_PULLUP);

    for (uint8_t c = 0; c < Pin::MOTOR_COUNT; ++c) {
        pinMode(in1(c), OUTPUT);
        pinMode(in2(c), OUTPUT);
        coast(c);
    }
}

void MotorDriver::enable()  { digitalWrite(Pin::MOTOR_nSLEEP, HIGH); }
void MotorDriver::disable() { digitalWrite(Pin::MOTOR_nSLEEP, LOW); }

bool MotorDriver::isEnabled() const { return digitalRead(Pin::MOTOR_nSLEEP) == HIGH; }
bool MotorDriver::fault() const     { return digitalRead(Pin::MOTOR_nFAULT) == LOW; }

void MotorDriver::set(uint8_t channel, MotorState state) {
    if (channel >= Pin::MOTOR_COUNT) return;
    const uint8_t i1 = in1(channel);
    const uint8_t i2 = in2(channel);

    switch (state) {
        case MotorState::Coast:   digitalWrite(i1, LOW);  digitalWrite(i2, LOW);  break;
        case MotorState::Forward: digitalWrite(i1, HIGH); digitalWrite(i2, LOW);  break;
        case MotorState::Reverse: digitalWrite(i1, LOW);  digitalWrite(i2, HIGH); break;
        case MotorState::Brake:   digitalWrite(i1, HIGH); digitalWrite(i2, HIGH); break;
    }
}

void MotorDriver::coast(uint8_t c)   { set(c, MotorState::Coast); }
void MotorDriver::brake(uint8_t c)   { set(c, MotorState::Brake); }
void MotorDriver::forward(uint8_t c) { set(c, MotorState::Forward); }
void MotorDriver::reverse(uint8_t c) { set(c, MotorState::Reverse); }

uint8_t MotorDriver::in1(uint8_t c) const { return Pin::MOTOR_IN[c][0]; }
uint8_t MotorDriver::in2(uint8_t c) const { return Pin::MOTOR_IN[c][1]; }
