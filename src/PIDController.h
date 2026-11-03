#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

// ============================================================
// PIDController - PID com anti-windup e derivada na medicao
//
//   - Derivada sobre a MEDICAO (evita derivative kick)
//   - Anti-windup por travamento condicional do integral
//   - Saida saturada em [OUTPUT_MIN, OUTPUT_MAX]
//   - Integral saturado em [INTEGRAL_MIN, INTEGRAL_MAX]
// ============================================================

class PIDController {
public:
    PIDController();

    void begin(float kp, float ki, float kd);
    float update(float setpoint, float measurement, float dtSim);

    void reset();
    void resetIntegral();

    float getIntegral() const { return _integral; }
    float getLastOutput() const { return _lastOutput; }

private:
    float _kp;
    float _ki;
    float _kd;

    float _integral;
    float _lastMeasurement;
    float _lastOutput;
    bool  _firstRun;

    float clamp(float value, float lo, float hi) const;
};

#endif // PID_CONTROLLER_H