#include "PIDController.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
PIDController::PIDController()
    : _kp(0.0f), _ki(0.0f), _kd(0.0f),
      _integral(0.0f),
      _lastMeasured(0.0f),
      _output(0.0f),
      _hasLastMeasured(false) {
}

// ------------------------------------------------------------
// Configuracao dos ganhos
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
    _lastMeasured    = 0.0f;
    _output          = 0.0f;
    _hasLastMeasured = false;
}

// ------------------------------------------------------------
// Reset apenas do integral
// ------------------------------------------------------------
void PIDController::resetIntegral() {
    _integral = 0.0f;
}

// ------------------------------------------------------------
// Calculo do PID
// ------------------------------------------------------------
float PIDController::update(float setpoint, float measured, float dt) {

    // dt invalido: nao calcula, mantem a ultima saida
    if (dt <= 0.0f) {
        return _output;
    }

    const float error = setpoint - measured;

    // --- Termo proporcional ---
    const float pTerm = _kp * error;

    // --- Termo integral (com anti-windup por clamping) ---
    _integral += _ki * error * dt;
    _integral = clamp(_integral, PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);

    // --- Termo derivativo ---
    // Derivada sobre a medicao (nao sobre o erro) para evitar
    // "derivative kick" quando o setpoint muda de segmento.
    float dTerm = 0.0f;
    if (_hasLastMeasured) {
        const float dMeasured = (measured - _lastMeasured) / dt;
        dTerm = -_kd * dMeasured;
    }

    // --- Saida ---
    float out = pTerm + _integral + dTerm;
    out = clamp(out, PID_OUTPUT_MIN, PID_OUTPUT_MAX);

    // Anti-windup condicional: se a saida saturou, desfaz o
    // incremento integral que empurrou para fora do limite.
    if (out >= PID_OUTPUT_MAX && error > 0.0f) {
        _integral -= _ki * error * dt;
        _integral = clamp(_integral, PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);
    } else if (out <= PID_OUTPUT_MIN && error < 0.0f) {
        _integral -= _ki * error * dt;
        _integral = clamp(_integral, PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);
    }

    _lastMeasured    = measured;
    _hasLastMeasured = true;
    _output          = out;

    return _output;
}

// ------------------------------------------------------------
// Utilitario
// ------------------------------------------------------------
float PIDController::clamp(float value, float minVal, float maxVal) const {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}