#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// FaultDetector - Deteccao de falhas do forno
// V1: THERMOCOUPLE_OPEN, SENSOR_STUCK, OVER_TEMPERATURE
// Stubs (nao gerados no V1): SSR_STUCK_ON, HEATING_FAILURE
// ============================================================

class FaultDetector {
public:
    FaultDetector();

    // Avalia as condicoes atuais e retorna a falha detectada.
    // temperature : temperatura atual em degC
    // setpoint    : setpoint atual em degC
    // power       : potencia calculada pelo PID (0-100%)
    // ssrOn       : estado binario do SSR neste instante
    // dt          : delta de tempo SIMULADO em segundos
    FaultCode update(float temperature, float setpoint,
                     float power, bool ssrOn, float dt);

    // Limpa o estado interno (timers, ultima temperatura).
    // Chamado ao entrar em RESETTING.
    void reset();

    // Falha atualmente ativa (persistente ate reset()).
    FaultCode getActiveFault() const { return _activeFault; }

private:
    FaultCode _activeFault;
    float     _lastTemperature;
    float     _stuckTimer;      // segundos acumulados em suspeita
    bool      _hasLastTemp;     // false ate a primeira leitura valida

    // Avalia cada condicao isoladamente
    FaultCode checkValidity(float temperature) const;
    FaultCode checkOverTemperature(float temperature) const;
    FaultCode checkSensorStuck(float temperature, float setpoint,
                               float power, bool ssrOn, float dt);
};

#endif // FAULT_DETECTOR_H