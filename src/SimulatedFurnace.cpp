#include "SimulatedFurnace.h"
#include "Config.h"

// ------------------------------------------------------------
// Parametros do modelo termico
// ------------------------------------------------------------
#define SIM_K_HEAT   0.35f    // degC/s a 100% de potencia
#define SIM_K_LOSS   0.0025f  // 1/s
#define SIM_T_AMB    25.0f    // degC

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
SimulatedFurnace::SimulatedFurnace()
    : _temperature(SIM_T_AMB),
      _power(0.0f),
      _injectionActive(false),
      _injectedTemperature(0.0f) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void SimulatedFurnace::begin() {
    reset();
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------
void SimulatedFurnace::reset() {
    _temperature = SIM_T_AMB;
    _power = 0.0f;
    _injectionActive = false;
    _injectedTemperature = 0.0f;
}


// ------------------------------------------------------------
// Aplicacao de potencia
// ------------------------------------------------------------
void SimulatedFurnace::setPower(float powerPct, float dtSim) {
    
    // Satura a potencia
    if (powerPct < 0.0f)   powerPct = 0.0f;
    if (powerPct > 100.0f) powerPct = 100.0f;
    _power = powerPct;
    
    // dt invalido: nao integra
    if (dtSim <= 0.0f) {
        return;
    }
    
    // Injecao ativa: temperatura fixa, modelo nao evolui
    if (_injectionActive) {
        _temperature = _injectedTemperature;
        return;
    }

    // Modelo de primeira ordem
    const float heatIn  = (_power / 100.0f) * SIM_K_HEAT;
    const float heatOut = (_temperature - SIM_T_AMB) * SIM_K_LOSS;
    const float dT      = (heatIn - heatOut) * dtSim;

    _temperature += dT;

    // Piso: nao desce abaixo do ambiente
    if (_temperature < SIM_T_AMB) {
        _temperature = SIM_T_AMB;
    }
}

// ------------------------------------------------------------
// Leitura de temperatura
// ------------------------------------------------------------
float SimulatedFurnace::getTemperature() {
    return _temperature;
}

void SimulatedFurnace::injectTemperature(float temperature) {
    _injectionActive = true;
    _injectedTemperature = temperature;
    _temperature = temperature;
}

// ------------------------------------------------------------
// Injecao (teste)
// ------------------------------------------------------------
void SimulatedFurnace::clearInjection() {
    _injectionActive = false;
}

// ------------------------------------------------------------
// Temperatura ambiente
// ------------------------------------------------------------
float SimulatedFurnace::ambientTemperature() const {
    return SIM_T_AMB;
}
