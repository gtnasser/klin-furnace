#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// FaultDetector - Deteccao de falhas do forno
//
// Falhas ativas no V1:
//   THERMOCOUPLE_OPEN : leitura fora da faixa fisica
//   SENSOR_STUCK      : aquecendo, plano e longe do alvo
//   OVER_TEMPERATURE  : acima do limite de seguranca
//
// Reservadas para o V2: SSR_STUCK_ON, HEATING_FAILURE
// Tempo em SIMULADO.
// ============================================================

class FaultDetector {
public:
    FaultDetector();

    void reset();

    FaultCode update(float temperature, float setpoint,
                     float power, bool ssrOn, float dtSim);

private:
    float _stuckTimerS;
    float _lastTemperature;

    bool isReadingInvalid(float temperature) const;
    bool isOverTemperature(float temperature) const;
    bool isSensorStuck(float temperature, float setpoint,
                       float power, float dtSim);
};

#endif // FAULT_DETECTOR_H