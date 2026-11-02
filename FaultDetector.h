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
// Reservadas para o V2 (declaradas, nao geradas):
//   SSR_STUCK_ON      : requer realimentacao do SSR
//   HEATING_FAILURE   : requer leitura real de temperatura
//
// Tempo em SIMULADO: dtSim vem do KilnController.
// ============================================================

class FaultDetector {
public:
    FaultDetector();

    // Zera todo o estado interno. Chamar no inicio de cada perfil.
    void reset();

    // Avalia as condicoes e retorna a falha ativa (NONE se ok).
    // temperature : leitura atual em degC
    // setpoint    : setpoint atual em degC
    // power       : potencia aplicada (0-100)
    // ssrOn       : estado binario do SSR (nao usado no V1)
    // dtSim       : delta de tempo SIMULADO em segundos
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