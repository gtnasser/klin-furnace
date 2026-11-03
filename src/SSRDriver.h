#ifndef SSR_DRIVER_H
#define SSR_DRIVER_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// SSRDriver - Modulacao do SSR por time-proportioning
// Opera em TEMPO REAL.
// Guarda: _pin == PIN_UNASSIGNED nao escreve em GPIO nenhum.
// ============================================================

class SSRDriver {
public:
    SSRDriver();

    void begin(uint8_t pin);
    void update(float powerPct, uint32_t dtRealMs);
    void forceOff();
    void reset();

    bool  isOn() const { return _isOn; }
    bool  isForcedOff() const { return _forcedOff; }
    float getPower() const { return _power; }

private:
    uint8_t  _pin;
    float    _power;
    bool     _isOn;
    bool     _forcedOff;
    uint32_t _windowElapsedMs;

    void setOutput(bool on);
};

#endif // SSR_DRIVER_H