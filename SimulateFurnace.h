#ifndef SIMULATED_FURNACE_H
#define SIMULATED_FURNACE_H

#include <Arduino.h>
#include "IFurnace.h"

// ============================================================
// SimulatedFurnace - Modelo termico simplificado
// V1: usado no Wokwi. Nao depende de hardware.
//
// Modelo: primeira ordem com perda para o ambiente.
//   dT/dt = (P * K_GAIN) - ((T - T_AMB) * K_LOSS)
// ============================================================

class SimulatedFurnace : public IFurnace {
public:
    SimulatedFurnace();

    void  begin() override;
    float getTemperature() override;
    void  setPower(float power, float dt) override;
    void  shutdown() override;
    void  reset() override;

    // Injeta uma temperatura fixa (para testar SENSOR_STUCK).
    // Chamar clearFaultInjection() para voltar ao modelo normal.
    void injectTemperature(float tempC);
    void clearFaultInjection();
    bool isInjectionActive() const { return _injectActive; }

private:
    float _temperature;      // degC
    float _power;            // 0-100
    bool  _injectActive;
    float _injectedTemp;
};

#endif // SIMULATED_FURNACE_H