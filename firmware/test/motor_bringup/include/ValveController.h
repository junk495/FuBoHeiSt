#pragma once

#include <cstdint>
#include "MotorDriver.h"
#include "CurrentSense.h"
#include "RippleCounter.h"

// Nicht-blockierende Ventilsteuerung.
//
// Kommandos (calibrate/open/close/moveTo) setzen nur das Ziel und kehren sofort
// zurück. tick() muss in loop() regelmäßig aufgerufen werden und treibt die
// Zustandsmaschine. Der Hub je Ventil wird kalibriert und im NVS gespeichert.
//
// Referenz: 0 = "auf" (r-Endlage, harter Anschlag). Die Position wird je Kanal
// in Pulsen ab dieser Referenz verfolgt; die Referenz wird nur neu geholt, wenn
// sie verloren geht (Boot/Fehler/Timeout).
class ValveController {
public:
    // Ergebnis der zuletzt abgeschlossenen Fahrt.
    struct Result {
        bool done = false;          // ganze Operation abgeschlossen?
        bool ok = false;            // erfolgreich (Stall oder Ziel erreicht)
        bool stalled = false;       // Endlage per Stall erreicht
        bool targetReached = false; // Ziel-Pulse erreicht
        int  pcnt = 0;              // gefahrene Pulse
    };

    ValveController(MotorDriver& motor, CurrentSense& sense, RippleCounter& ripple);

    void begin();

    // --- Nicht-blockierende Kommandos (kehren sofort zurück) ---
    bool calibrate(uint8_t channel);              // Referenzlauf: f->zu, r->auf, Hub speichern
    bool open(uint8_t channel);                   // ganz auf (r bis Anschlag)
    bool close(uint8_t channel);                  // ganz zu (f bis Dichtung)
    bool moveTo(uint8_t channel, uint8_t percent);// X % zu (0 = auf, 100 = zu)

    // --- In loop() aufrufen ---
    void tick();

    // --- Status ---
    bool busy() const;                            // läuft gerade eine Operation?
    Result result() const;                        // Ergebnis der letzten Operation
    int32_t position(uint8_t channel) const;      // aktuelle Position (Pulse ab "auf")
    uint32_t stroke(uint8_t channel) const;       // kalibrierter Hub (0 = nicht kalibriert)
    bool hasReference(uint8_t channel) const;     // Referenz gültig?

    // --- Parameter ---
    void setTimeoutMs(unsigned long ms)        { timeoutMs_ = ms; }
    void setSampleIntervalMs(unsigned long ms) { sampleIntervalMs_ = ms; }
    void setOpenStallMa(float ma)              { openStallMa_ = ma; }
    void setCloseStallMa(float ma)             { closeStallMa_ = ma; }

private:
    static constexpr uint8_t MAX_CHANNELS = 8;

    enum class Op : uint8_t { Idle, Move, RefThenMove, Calibrate };
    enum class Phase : uint8_t { Enable, Driving, Retry, Finishing };

    void startDrive(MotorState dir, float stallMa, int32_t targetPulses);
    void driveSample(unsigned long now);
    void finish(bool stalled, bool targetReached);
    void complete();

    MotorDriver& motor_;
    CurrentSense& sense_;
    RippleCounter& ripple_;

    // Parameter
    unsigned long timeoutMs_       = 60000;
    unsigned long sampleIntervalMs_ = 10;
    unsigned long stallDebounceMs_ = 100;
    unsigned long stallBlankMs_    = 400;
    unsigned long startWatchdogMs_ = 1000;
    float  startWatchdogMa_        = 10.0f;
    int    maxStartRetries_        = 3;
    float  openStallMa_            = 60.0f;
    float  closeStallMa_           = 50.0f;

    // je Kanal
    int32_t  pos_[MAX_CHANNELS] = {};
    bool     ref_[MAX_CHANNELS] = {};
    uint32_t stroke_[MAX_CHANNELS] = {};

    // aktive Operation
    Op        op_ = Op::Idle;
    Phase     phase_ = Phase::Enable;
    uint8_t   channel_ = 0;
    MotorState dir_ = MotorState::Coast;
    float     stallMa_ = 0.0f;
    int32_t   targetPulses_ = -1;
    int32_t   startPos_ = 0;
    int       calibStep_ = 0;      // 0 = schließen, 1 = öffnen
    uint8_t   pendingPercent_ = 0; // für RefThenMove

    unsigned long start_ = 0;
    unsigned long phaseStart_ = 0;
    unsigned long lastSample_ = 0;
    unsigned long stallSince_ = 0;
    unsigned long lastDriveMs_ = 0;
    int startRetries_ = 0;

    Result result_;
};
