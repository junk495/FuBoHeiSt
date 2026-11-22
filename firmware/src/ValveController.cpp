#include "ValveController.h"
#include <Preferences.h>
#include <WString.h>

namespace {
// NVS-Namespace bewusst identisch zum Bringup (valveCalibrate): so werden
// dort kalibrierte Hübe (stroke<N>) auch in der Haupt-Firmware erkannt.
constexpr const char* NVS_NAMESPACE = "motorlog";
}  // namespace

ValveController::ValveController(MotorDriver& motor, CurrentSense& sense, RippleCounter& ripple)
    : motor_(motor), sense_(sense), ripple_(ripple) {}

void ValveController::begin() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    for (uint8_t c = 0; c < MAX_CHANNELS; c++) {
        stroke_[c] = prefs.getInt(("stroke" + String(c)).c_str(), 0);
        pos_[c] = 0;
        ref_[c] = false;
    }
    prefs.end();
}

bool ValveController::calibrate(uint8_t channel) {
    if (op_ != Op::Idle || channel >= MAX_CHANNELS) return false;
    channel_ = channel;
    op_ = Op::Calibrate;
    calibStep_ = 0;
    startDrive(MotorState::Forward, closeStallMa_, -1);  // f->zu (Dichtung)
    return true;
}

bool ValveController::open(uint8_t channel) {
    if (op_ != Op::Idle || channel >= MAX_CHANNELS) return false;
    channel_ = channel;
    op_ = Op::Move;
    startDrive(MotorState::Reverse, openStallMa_, -1);   // r->auf (Anschlag)
    return true;
}

bool ValveController::close(uint8_t channel) {
    if (op_ != Op::Idle || channel >= MAX_CHANNELS) return false;
    channel_ = channel;
    op_ = Op::Move;
    startDrive(MotorState::Forward, closeStallMa_, -1);  // f->zu (Dichtung)
    return true;
}

bool ValveController::moveTo(uint8_t channel, uint8_t percent) {
    if (op_ != Op::Idle || channel >= MAX_CHANNELS) return false;
    if (percent == 0) return open(channel);
    if (percent >= 100) return close(channel);

    if (stroke_[channel] == 0) return false;   // nicht kalibriert

    channel_ = channel;

    if (!ref_[channel]) {
        // erst Referenz holen (auf), dann Ziel anfahren
        op_ = Op::RefThenMove;
        pendingPercent_ = percent;
        startDrive(MotorState::Reverse, openStallMa_, -1);
        return true;
    }

    const int32_t target = (int32_t)stroke_[channel] * percent / 100;
    const int32_t delta  = target - pos_[channel];
    if (delta == 0) return true;               // bereits dort

    op_ = Op::Move;
    if (delta > 0) startDrive(MotorState::Forward, closeStallMa_, delta);
    else           startDrive(MotorState::Reverse, openStallMa_, -delta);
    return true;
}

void ValveController::tick() {
    if (op_ == Op::Idle) return;
    const unsigned long now = millis();

    switch (phase_) {
        case Phase::Enable:
            if (now - phaseStart_ >= 2) {
                motor_.set(channel_, dir_);
                lastDriveMs_ = now;
                phase_ = Phase::Driving;
            }
            break;

        case Phase::Driving:
            if (now - start_ >= timeoutMs_) { finish(false, false); break; }
            if (motor_.fault())             { finish(false, false); break; }
            if (now - lastSample_ >= sampleIntervalMs_) {
                lastSample_ = now;
                driveSample(now);
            }
            break;

        case Phase::Retry:
            if (now - phaseStart_ >= 50) {
                motor_.enable();
                phase_ = Phase::Enable;
                phaseStart_ = now;
            }
            break;

        case Phase::Finishing:
            if (now - phaseStart_ >= 100) complete();
            break;
    }
}

bool ValveController::busy() const { return op_ != Op::Idle; }
ValveController::Result ValveController::result() const { return result_; }
int32_t  ValveController::position(uint8_t channel) const { return (channel < MAX_CHANNELS) ? pos_[channel] : 0; }
uint32_t ValveController::stroke(uint8_t channel) const { return (channel < MAX_CHANNELS) ? stroke_[channel] : 0; }
bool     ValveController::hasReference(uint8_t channel) const { return (channel < MAX_CHANNELS) && ref_[channel]; }

void ValveController::startDrive(MotorState dir, float stallMa, int32_t targetPulses) {
    dir_ = dir;
    stallMa_ = stallMa;
    targetPulses_ = targetPulses;
    startPos_ = pos_[channel_];
    start_ = millis();
    lastSample_ = 0;
    lastDriveMs_ = start_;
    stallSince_ = 0;
    startRetries_ = 0;
    motor_.enable();
    ripple_.clear();
    phase_ = Phase::Enable;
    phaseStart_ = start_;
}

void ValveController::driveSample(unsigned long now) {
    const bool fresh = sense_.read();
    const int pulses = ripple_.count();

    if (targetPulses_ >= 0 && pulses >= targetPulses_) {
        finish(false, true);
        return;
    }

    if (!fresh) return;

    const float ma = sense_.currentMilliamps();

    // Stall-Erkennung erst nach der Blindzeit (Anlaufstrom ignorieren).
    if (now - start_ >= stallBlankMs_) {
        if (ma > stallMa_) {
            if (stallSince_ == 0) stallSince_ = now;
            else if (now - stallSince_ >= stallDebounceMs_) {
                finish(true, false);
                return;
            }
        } else {
            stallSince_ = 0;
        }
    }

    // Start-Watchdog: Motor läuft nicht an -> nSLEEP toggeln und erneut versuchen.
    if (startRetries_ < maxStartRetries_ &&
        now - lastDriveMs_ >= startWatchdogMs_ &&
        ma < startWatchdogMa_) {
        startRetries_++;
        motor_.disable();
        phase_ = Phase::Retry;
        phaseStart_ = now;
        stallSince_ = 0;
    }
}

void ValveController::finish(bool stalled, bool targetReached) {
    motor_.brake(channel_);
    motor_.disable();
    result_.done = false;
    result_.ok = stalled || targetReached;
    result_.stalled = stalled;
    result_.targetReached = targetReached;
    result_.pcnt = ripple_.count();
    phase_ = Phase::Finishing;
    phaseStart_ = millis();
}

void ValveController::complete() {
    // Position aktualisieren
    if (result_.stalled) {
        if (dir_ == MotorState::Reverse) {
            pos_[channel_] = 0;
            ref_[channel_] = true;
        } else {
            if (stroke_[channel_] > 0) pos_[channel_] = stroke_[channel_];
            ref_[channel_] = true;
        }
    } else if (result_.targetReached) {
        if (dir_ == MotorState::Forward) pos_[channel_] = startPos_ + result_.pcnt;
        else                            pos_[channel_] = startPos_ - result_.pcnt;
        if (pos_[channel_] < 0) pos_[channel_] = 0;
        if (stroke_[channel_] > 0 && pos_[channel_] > (int32_t)stroke_[channel_])
            pos_[channel_] = stroke_[channel_];
        ref_[channel_] = true;
    } else {
        ref_[channel_] = false;   // Timeout/Fehler -> Referenz verloren
    }

    // Operation fortschreiben/abschließen
    switch (op_) {
        case Op::Move:
            op_ = Op::Idle;
            result_.done = true;
            break;

        case Op::RefThenMove:
            if (result_.stalled) {
                // Referenz (auf) geholt -> jetzt Ziel anfahren
                const int32_t target = (int32_t)stroke_[channel_] * pendingPercent_ / 100;
                const int32_t delta  = target - pos_[channel_];  // pos_ = 0
                op_ = Op::Move;
                if (delta > 0) startDrive(MotorState::Forward, closeStallMa_, delta);
                else           { op_ = Op::Idle; result_.done = true; }
            } else {
                op_ = Op::Idle;
                result_.done = true;
            }
            break;

        case Op::Calibrate:
            if (calibStep_ == 0 && result_.stalled) {
                calibStep_ = 1;
                startDrive(MotorState::Reverse, openStallMa_, -1);
            } else if (calibStep_ == 1 && result_.stalled) {
                stroke_[channel_] = result_.pcnt;
                pos_[channel_] = 0;
                ref_[channel_] = true;
                Preferences prefs;
                prefs.begin(NVS_NAMESPACE, false);
                prefs.putInt(("stroke" + String(channel_)).c_str(), result_.pcnt);
                prefs.end();
                op_ = Op::Idle;
                result_.done = true;
            } else {
                op_ = Op::Idle;
                result_.done = true;
            }
            break;

        case Op::Idle:
            break;
    }
}
