#include "StatusLeds.h"
#include "Config.h"

// ------------------------------------------------------------
// Parametros de pisca
// ------------------------------------------------------------
#define BLINK_PERIOD_MS  500

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
StatusLeds::StatusLeds()
    : _runPin(PIN_UNASSIGNED),
      _faultPin(PIN_UNASSIGNED),
      _runOn(false),
      _faultOn(false),
      _blinkTimerMs(0),
      _blinkPhase(false) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void StatusLeds::begin(uint8_t runPin, uint8_t faultPin) {
    _runPin   = runPin;
    _faultPin = faultPin;

    if (_runPin != PIN_UNASSIGNED) {
        pinMode(_runPin, OUTPUT);
    }
    if (_faultPin != PIN_UNASSIGNED) {
        pinMode(_faultPin, OUTPUT);
    }

    _runOn        = false;
    _faultOn      = false;
    _blinkTimerMs = 0;
    _blinkPhase   = false;

    setRun(false);
    setFault(false);
}

// ------------------------------------------------------------
// Atualizacao
// ------------------------------------------------------------
void StatusLeds::update(KilnState state, FaultCode fault, uint32_t dtMs) {

    // Avanca o timer de pisca

    _blinkTimerMs += dtMs;
    if (_blinkTimerMs >= BLINK_PERIOD_MS) {
        _blinkTimerMs = 0;
        _blinkPhase = !_blinkPhase;
    }

    // --- LED de falha ---
    if (fault != FaultCode::NONE) {
        setFault(_blinkPhase);
    } else {
        setFault(false);
    }

    // --- LED de run ---
    switch (state) {
        case KilnState::IDLE:      setRun(false); break;
        case KilnState::STARTING:  setRun(_blinkPhase); break;
        case KilnState::RUNNING:   setRun(true); break;
        case KilnState::FINISHED:  setRun(true); break;
        case KilnState::RESETTING: setRun(_blinkPhase); break;
        case KilnState::EMERGENCY: setRun(false); break;
        default:                   setRun(false); break;
    }
}

// ------------------------------------------------------------
// Desliga tudo
// ------------------------------------------------------------
void StatusLeds::allOff() {
    setRun(false);
    setFault(false);
}

// ------------------------------------------------------------
// Escrita nos pinos (com guarda e cache)
// ------------------------------------------------------------
void StatusLeds::setRun(bool on) {
    if (_runPin == PIN_UNASSIGNED) {
        _runOn = false;
        return;
    }
    if (_runOn == on) {
        return;
    }
    _runOn = on;
    digitalWrite(_runPin, on ? HIGH : LOW);
}

void StatusLeds::setFault(bool on) {
    if (_faultPin == PIN_UNASSIGNED) {
        _faultOn = false;
        return;
    }
    if (_faultOn == on) {
        return;
    }
    _faultOn = on;
    digitalWrite(_faultPin, on ? HIGH : LOW);
}
