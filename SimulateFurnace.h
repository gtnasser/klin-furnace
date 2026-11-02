#ifndef SIMULATED_FURNACE_H
#define SIMULATED_FURNACE_H

#include <Arduino.h>
#include "IFurnace.h"

// ============================================================
// SimulatedFurnace - Modelo termico de primeira ordem
//
// dT/dt = (P * K_HEAT) - ((T - T_AMB) * K_LOSS)
//
// Onde:
//   P       : potencia media (0-100)
//   K_HEAT  : ganho de aquecimento (degC/s a 100%)
//   K_LOSS  : coeficiente de perda (1/s)
//   T_AMB   : temperatura ambiente
//
// Tempo em SIMULADO: dtSim vem do KilnController.
// O modelo nao tem inercia propria: a inercia emerge da
// constante de tempo 1/K_LOSS.
// ============================================================

class SimulatedFurnace : public IFurnace {
public:
    SimulatedFurnace();

    void begin() override;
    void setPower(float powerPct, float dtSim) override;
    float getTemperature() override;
    void reset() override;

    // Injecao de temperatura (apenas para teste).
    // Usado para simular sensor travado ou leitura invalida.
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