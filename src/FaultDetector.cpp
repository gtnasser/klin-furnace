#include "FaultDetector.h"
#include "Config.h"
#include <math.h>

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
FaultDetector::FaultDetector()
    : _stuckTimerS(0.0f),
      _lastTemperature(0.0f) {
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------
void FaultDetector::reset() {
    _stuckTimerS = 0.0f;
    _lastTemperature = 0.0f;
}

// ------------------------------------------------------------
// Avaliacao
// ------------------------------------------------------------
FaultCode FaultDetector::update(float temperature, float setpoint,
                                float power, bool ssrOn, float dtSim) {

    (void)ssrOn;   // V2: deteccao de SSR travado exige realimentacao

    if (isReadingInvalid(temperature)) {
        return FaultCode::THERMOCOUPLE_OPEN;
    }

    if (isOverTemperature(temperature)) {
        return FaultCode::OVER_TEMPERATURE;
    }

    if (isSensorStuck(temperature, setpoint, power, dtSim)) {
        return FaultCode::SENSOR_STUCK;
    }

    return FaultCode::NONE;
}

// ------------------------------------------------------------
// Leitura invalida
// Fora da faixa fisica plausivel em qualquer direcao.
// ------------------------------------------------------------
bool FaultDetector::isReadingInvalid(float temperature) const {
    if (isnan(temperature) || isinf(temperature)) {
        return true;
    }
    if (temperature < MIN_VALID_TEMP_C) {
        return true;
    }
    if (temperature > (SAFETY_LIMIT_C + 200.0f)) {
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// Sobretemperatura
// ------------------------------------------------------------
bool FaultDetector::isOverTemperature(float temperature) const {
    return temperature > SAFETY_LIMIT_C;
}

// ------------------------------------------------------------
// Sensor travado
// Tres condicoes simultaneas:
//   - aquecendo (potencia acima do limiar)
//   - leitura plana (variacao abaixo do limiar)
//   - longe do alvo (erro acima da banda)
// ------------------------------------------------------------
bool FaultDetector::isSensorStuck(float temperature, float setpoint,
                                  float power, float dtSim) {

    const bool heating = (power >= STUCK_POWER_THRESHOLD);
    const bool farFromTarget =
        (fabsf(setpoint - temperature) > STUCK_SETPOINT_BAND);

    if (!heating || !farFromTarget) {
        _stuckTimerS = 0.0f;
        _lastTemperature = temperature;
        return false;
    }

    const float delta = fabsf(temperature - _lastTemperature);
    _lastTemperature = temperature;

    if (delta < STUCK_DELTA_THRESHOLD) {
        _stuckTimerS += dtSim;
    } else {
        _stuckTimerS = 0.0f;
    }

    return (_stuckTimerS >= STUCK_WINDOW_S);
}