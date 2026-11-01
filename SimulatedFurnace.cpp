#include "SimulatedFurnace.h"
#include "Config.h"

// ------------------------------------------------------------
// Parametros do modelo termico
// ------------------------------------------------------------
// Ganho de aquecimento: degC/s com 100% de potencia.
// Escolhido para que o forno atinja ~900 degC em ~30 min
// de tempo simulado com potencia plena.
#define SIM_AMBIENT_C        25.0f
#define SIM_HEAT_GAIN        0.55f    // degC/s a 100%
#define SIM_LOSS_COEF        0.00035f // perda proporcional ao erro
#define SIM_MAX_TEMP_C       1300.0f  // teto de seguranca do modelo

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
SimulatedFurnace::SimulatedFurnace()
    : _temperature(SIM_AMBIENT_C),
      _power(0.0f),
      _injectActive(false),
      _injectedTemp(0.0f) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void SimulatedFurnace::begin() {
    _temperature  = SIM_AMBIENT_C;
    _power        = 0.0f;
    _injectActive = false;
    _injectedTemp = 0.0f;
}

// ------------------------------------------------------------
// Leitura de temperatura
// ------------------------------------------------------------
float SimulatedFurnace::getTemperature() {
    if (_injectActive) {
        return _injectedTemp;
    }
    return _temperature;
}

// ------------------------------------------------------------
// Aplicacao de potencia
// ------------------------------------------------------------
void SimulatedFurnace::setPower(float power, float dt) {
    if (power < 0.0f)   power = 0.0f;
    if (power > 100.0f) power = 100.0f;
    _power = power;

    if (dt <= 0.0f) {
        return;
    }

    // Termo de aquecimento (proporcional a potencia)
    const float heat = (_power / 100.0f) * SIM_HEAT_GAIN;

    // Termo de perda (proporcional a diferenca para o ambiente)
    const float loss = (_temperature - SIM_AMBIENT_C) * SIM_LOSS_COEF;

    // Integracao de Euler
    _temperature += (heat - loss) * dt;

    // Teto do modelo
    if (_temperature > SIM_MAX_TEMP_C) {
        _temperature = SIM_MAX_TEMP_C;
    }
    // Piso: nunca abaixo do ambiente
    if (_temperature < SIM_AMBIENT_C) {
        _temperature = SIM_AMBIENT_C;
    }
}

// ------------------------------------------------------------
// Desligamento
// ------------------------------------------------------------
void SimulatedFurnace::shutdown() {
    _power = 0.0f;
    // Nao zera a temperatura: o forno fisico tambem nao esfria
    // instantaneamente. O resfriamento natural continua no
    // proximo setPower().
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------
void SimulatedFurnace::reset() {
    _temperature  = SIM_AMBIENT_C;
    _power        = 0.0f;
    _injectActive = false;
    _injectedTemp = 0.0f;
}

// ------------------------------------------------------------
// Injecao de temperatura (teste de SENSOR_STUCK)
// ------------------------------------------------------------
void SimulatedFurnace::injectTemperature(float tempC) {
    _injectActive = true;
    _injectedTemp = tempC;
}

void SimulatedFurnace::clearFaultInjection() {
    _injectActive = false;
    _injectedTemp = 0.0f;
}