#include "StatusLeds.h"
#include "Config.h"

// ------------------------------------------------------------
// Parametros de pisca
// ------------------------------------------------------------
#define BLINK_PERIOD_MS  500   // meio periodo do pisca

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
StatusLeds::StatusLeds()
    : _runPin(0),
      _faultPin(0),
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

    pinMode(_runPin, OUTPUT);
    pinMode(_faultPin, OUTPUT);

    setRun(false);
    setFault(false);

    _blinkTimerMs = 0;
    _blinkPhase   = false;
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
    // Pisca quando ha falha ativa; apagado caso contrario.
    if (fault != FaultCode::NONE) {
        setFault(_blinkPhase);
    } else {
        setFault(false);
    }

    // --- LED de run ---
    switch (state) {
        case KilnState::IDLE:
            // Apagado: aguardando comando
            setRun(false);
            break;

        case KilnState::STARTING:
            // Pisca rapido: preparando
            setRun(_blinkPhase);
            break;

        case KilnState::RUNNING:
            // Aceso continuo: operando
            setRun(true);
            break;

        case KilnState::FINISHED:
            // Aceso continuo: perfil concluido, forno ainda quente
            setRun(true);
            break;

        case KilnState::RESETTING:
            // Pisca: retornando ao estado seguro
            setRun(_blinkPhase);
            break;

        case KilnState::EMERGENCY:
            // Apagado: forno em falha critica
            setRun(false);
            break;

        default:
            setRun(false);
            break;
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
// Escrita nos pinos (com cache para evitar digitalWrite repetido)
// ------------------------------------------------------------
void StatusLeds::setRun(bool on) {
    if (_runOn == on) {
        return;
    }
    _runOn = on;
    digitalWrite(_runPin, on ? HIGH : LOW);
}

void StatusLeds::setFault(bool on) {
    if (_faultOn == on) {
        return;
    }
    _faultOn = on;
    digitalWrite(_faultPin, on ? HIGH : LOW);
}