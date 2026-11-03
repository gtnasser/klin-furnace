#ifndef SIMULATED_FURNACE_H
#define SIMULATED_FURNACE_H

#include <Arduino.h>
#include "IFurnace.h"

// ============================================================
// SimulatedFurnace - Modelo termico de primeira ordem
//
// dT/dt = (P * K_HEAT) - ((T - T_AMB) * K_LOSS)
// Tempo em SIMULADO.
// ============================================================

class SimulatedFurnace : public IFurnace {
public:
    SimulatedFurnace();

    void  begin() override;
    void  setPower(float powerPct, float dtSim) override;
    float getTemperature() override;
    void  reset() override;

    // Injecao de temperatura (apenas para teste)
    void injectTemperature(float temperature);
    void clearInjection();

private:
    float _temperature;
    float _power;
    bool  _injectionActive;
    float _injectedTemperature;

    float ambientTemperature() const;
};

#endif // SIMULATED_FURNACE_H