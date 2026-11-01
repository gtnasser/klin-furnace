#include "SSRDriver.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
SSRDriver::SSRDriver()
    : _pin(0),
      _power(0.0f),
      _isOn(false),
      _forcedOff(true),        // comeca travado ate begin()
      _windowElapsedMs(0) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void SSRDriver::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    setOutput(false);

    _power            = 0.0f;
    _isOn             = false;
    _forcedOff        = false;   // libera apos configurar o pino
    _windowElapsedMs  = 0;
}

// ------------------------------------------------------------
// Atualizacao da modulacao
// ------------------------------------------------------------
void SSRDriver::update(float power, uint32_t dtMs) {

    // Travado por emergencia: garante OFF e ignora comandos
    if (_forcedOff) {
        setOutput(false);
        return;
    }

    // Sanitiza a potencia
    if (power < 0.0f)   power = 0.0f;
    if (power > 100.0f) power = 100.0f;
    _power = power;

    // Potencia zero: desliga e reinicia a janela
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
    _windowElapsedMs += dtMs;
    if (_windowElapsedMs >= SSR_WINDOW_MS) {
        _windowElapsedMs %= SSR_WINDOW_MS;
    }

    // Tempo ligado proporcional a potencia
    uint32_t onTimeMs = (uint32_t)((SSR_WINDOW_MS * _power) / 100.0f);

    // Pulsos muito curtos nao acionam o SSR de forma confiavel
    if (onTimeMs > 0 && onTimeMs < SSR_MIN_ON_MS) {
        onTimeMs = SSR_MIN_ON_MS;
    }

    setOutput(_windowElapsedMs < onTimeMs);
}

// ------------------------------------------------------------
// Desligamento forcado (emergencia)
// ------------------------------------------------------------
void SSRDriver::forceOff() {
    _forcedOff = true;
    _power     = 0.0f;
    _windowElapsedMs = 0;
    setOutput(false);
}

// ------------------------------------------------------------
// Libera o travamento
// ------------------------------------------------------------
void SSRDriver::reset() {
    _forcedOff       = false;
    _power           = 0.0f;
    _isOn            = false;
    _windowElapsedMs = 0;
    setOutput(false);
}

// ------------------------------------------------------------
// Escrita no pino
// ------------------------------------------------------------
void SSRDriver::setOutput(bool on) {
    _isOn = on;
    digitalWrite(_pin, on ? HIGH : LOW);
}