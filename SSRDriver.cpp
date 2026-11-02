#include "SSRDriver.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
SSRDriver::SSRDriver()
    : _pin(PIN_UNASSIGNED),
      _power(0.0f),
      _isOn(false),
      _forcedOff(true),
      _windowElapsedMs(0) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void SSRDriver::begin(uint8_t pin) {
    _pin = pin;

    if (_pin == PIN_UNASSIGNED) {
        _isOn = false;
        _forcedOff = true;
        return;
    }

    pinMode(_pin, OUTPUT);
    setOutput(false);

    _power = 0.0f;
    _isOn = false;
    _forcedOff = false;
    _windowElapsedMs = 0;
}

// ------------------------------------------------------------
// Modulacao por time-proportioning
// ------------------------------------------------------------
void SSRDriver::update(float powerPct, uint32_t dtRealMs) {

    // Sem pino ou travado: saida permanece desligada
    if (_pin == PIN_UNASSIGNED || _forcedOff) {
        setOutput(false);
        return;
    }

    // Satura a potencia na faixa valida
    if (powerPct < 0.0f)   powerPct = 0.0f;
    if (powerPct > 100.0f) powerPct = 100.0f;
    _power = powerPct;

    // Potencia nula: desliga e reinicia a janela
    if (_power <= 0.0f) {
        setOutput(false);
        _windowElapsedMs = 0;
        return;
    }

    // Potencia total: liga continuamente
    if (_power >= 100.0f) {
        setOutput(true);
        _windowElapsedMs = 0;
        return;
    }

    // Avanca a janela
    _windowElapsedMs += dtRealMs;
    if (_windowElapsedMs >= SSR_WINDOW_MS) {
        _windowElapsedMs -= SSR_WINDOW_MS;
    }

    // Tempo ligado dentro da janela
    const uint32_t onTimeMs =
        (uint32_t)((SSR_WINDOW_MS * _power) / 100.0f);

    // Pulsos muito curtos sao descartados (confiabilidade do SSR)
    if (onTimeMs < SSR_MIN_ON_MS) {
        setOutput(false);
        return;
    }

    setOutput(_windowElapsedMs < onTimeMs);
}

// ------------------------------------------------------------
// Desligamento forcado
// ------------------------------------------------------------
void SSRDriver::forceOff() {
    _forcedOff = true;
    _power = 0.0f;
    _windowElapsedMs = 0;
    setOutput(false);
}

// ------------------------------------------------------------
// Libera o travamento
// ------------------------------------------------------------
void SSRDriver::reset() {
    _forcedOff = false;
    _power = 0.0f;
    _windowElapsedMs = 0;
    setOutput(false);
}

// ------------------------------------------------------------
// Escrita no pino (com guarda)
// ------------------------------------------------------------
void SSRDriver::setOutput(bool on) {
    _isOn = on;

    if (_pin == PIN_UNASSIGNED) {
        return;   // sem pino: nao escreve em GPIO nenhum
    }

    digitalWrite(_pin, on ? HIGH : LOW);
}