#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

// ============================================================
// PIDController - Controle PID com anti-windup
// Saida em percentual de potencia (0-100%)
// ============================================================

class PIDController {
public:
    PIDController();

    // Configura os ganhos. Chamar uma vez no setup.
    void begin(float kp, float ki, float kd);

    // Calcula a potencia de saida.
    // setpoint    : alvo em degC
    // measured    : temperatura atual em degC
    // dt          : delta de tempo SIMULADO em segundos
    // Retorna potencia 0-100%.
    float update(float setpoint, float measured, float dt);

    // Zera o estado interno (integral e derivada).
    // Chamado ao entrar em RESETTING ou ao iniciar um novo perfil.
    void reset();

    // Zera apenas o acumulador integral.
    // Util em mudancas de etapa do perfil.
    void resetIntegral();

    float getOutput() const { return _output; }
    float getIntegral() const { return _integral; }

private:
    float _kp, _ki, _kd;
    float _integral;
    float _lastMeasured;
    float _output;
    bool  _hasLastMeasured;

    float clamp(float value, float minVal, float maxVal) const;
};

#endif // PID_CONTROLLER_H