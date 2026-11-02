#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

// ============================================================
// PIDController - PID com anti-windup e derivada na medicao
//
// Decisoes:
//   - Derivada sobre a MEDICAO (evita derivative kick)
//   - Anti-windup por travamento condicional do integral
//   - Saida saturada em [OUTPUT_MIN, OUTPUT_MAX]
//   - Integral saturado em [INTEGRAL_MIN, INTEGRAL_MAX]
//
// Tempo em SIMULADO: dtSim vem do KilnController.
// ============================================================

class PIDController {
public:
    PIDController();

    // Configura os ganhos. Chamar uma vez no setup.
    void begin(float kp, float ki, float kd);

    // Calcula a saida do PID.
    // setpoint    : alvo em degC
    // measurement : temperatura atual em degC
    // dtSim       : delta de tempo SIMULADO em segundos
    // Retorna potencia em [OUTPUT_MIN, OUTPUT_MAX].
    float update(float setpoint, float measurement, float dtSim);

    // Zera integral, derivada e estado de primeira execucao.
    void reset();

    // Zera apenas o integral (usado na troca de segmento).
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