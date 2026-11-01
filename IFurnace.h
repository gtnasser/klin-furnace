#ifndef IFURNACE_H
#define IFURNACE_H

#include <Arduino.h>

// ============================================================
// IFurnace - Interface abstrata do forno
// Unico ponto de troca entre simulacao (V1) e hardware (V2)
// ============================================================

class IFurnace {
public:
    virtual ~IFurnace() {}

    // Inicializa o forno. Chamar uma vez no setup.
    virtual void begin() = 0;

    // Temperatura atual em degC.
    // Retorna NAN se a leitura for invalida.
    virtual float getTemperature() = 0;

    // Aplica potencia media (0-100%).
    // dt : delta de tempo SIMULADO em segundos
    virtual void setPower(float power, float dt) = 0;

    // Desligamento imediato. Usado em EMERGENCY.
    virtual void shutdown() = 0;

    // Reinicia o modelo/estado interno. Usado em RESETTING.
    virtual void reset() = 0;
};

#endif // IFURNACE_H