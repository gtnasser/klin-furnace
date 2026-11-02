#include "PIDController.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
PIDController::PIDController()
    : _kp(0.0f),
      _ki(0.0f),
      _kd(0.0f),
      _integral(0.0f),
      _lastMeasurement(0.0f),
      _lastOutput(0.0f),
      _firstRun(true) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void PIDController::begin(float kp, float ki, float kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
    reset();
}

// ------------------------------------------------------------
// Reset completo
// ------------------------------------------------------------
void PIDController::reset() {
    _integral        = 0.0f;
    _lastMeasurement = 0.0f;
    _lastOutput      = 0.0f;
    _firstRun        = true;
}

// ------------------------------------------------------------
// Reset do integral (troca de segmento)
// ------------------------------------------------------------
void PIDController::resetIntegral() {
    _integral = 0.0f;
}

// ------------------------------------------------------------
// Calculo do PID
// ------------------------------------------------------------
float PIDController::update(float setpoint, float measurement, float dtSim) {

    // dt invalido: retorna a ultima saida sem alterar estado
    if (dtSim <= 0.0f) {
        return _lastOutput;
    }

    const float error = setpoint - measurement;

    // --- Proporcional ---
    const float pTerm = _kp * error;

    // --- Derivada sobre a MEDICAO ---
    // Evita derivative kick quando o setpoint muda de segmento.
    float dTerm = 0.0f;
    if (_firstRun) {
        _lastMeasurement = measurement;
        _firstRun = false;
    } else {
        const float dMeas = (measurement - _lastMeasurement) / dtSim;
        dTerm = -_kd * dMeas;
        _lastMeasurement = measurement;
    }

    // --- Integral com anti-windup condicional ---
    // Nao integra se a saida ja esta saturada na mesma direcao do erro.
    const bool saturatedHigh = (_lastOutput >= PID_OUTPUT_MAX);
    const bool saturatedLow  = (_lastOutput <= PID_OUTPUT_MIN);

    const bool blockIntegral =
        (saturatedHigh && error > 0.0f) ||
        (saturatedLow  && error < 0.0f);

    if (!blockIntegral) {
        _integral += _ki * error * dtSim;
        _integral = clamp(_integral, PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);
    }

    // --- Saida ---
    const float output = pTerm + _integral + dTerm;
    _lastOutput = clamp(output, PID_OUTPUT_MIN, PID_OUTPUT_MAX);

    return _lastOutput;
}

// ------------------------------------------------------------
// Saturação
// ------------------------------------------------------------
float PIDController::clamp(float value, float lo, float hi) const {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}