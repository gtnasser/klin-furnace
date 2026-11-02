#ifndef IFURNACE_H
#define IFURNACE_H

#include <Arduino.h>

// ============================================================
// IFurnace - Interface do forno
//
// Unico ponto de troca entre simulacao e hardware real.
// V1: SimulatedFurnace (modelo termico em software)
// V2: RealFurnace (termopar MAX31855 + SSR fisico)
//
// Contrato:
//   - setPower() recebe potencia MEDIA (0-100), nao o binario
//     do SSR. O modelo termico integra a potencia ao longo do
//     tempo; o SSR modula por conta propria.
//   - dtSim e tempo SIMULADO em segundos.
//   - getTemperature() retorna degC, ou NAN se a leitura falhar.
// ============================================================

class IFurnace {
public:
    virtual ~IFurnace() {}

    // Inicializa o hardware ou o modelo. Chamar uma vez no setup.
    virtual void begin() = 0;

    // Aplica potencia media (0-100) por dtSim segundos simulados.
    virtual void setPower(float powerPct, float dtSim) = 0;

    // Le a temperatura atual em degC. NAN se invalida.
    virtual float getTemperature() = 0;

    // Reinicia o modelo/hardware ao estado inicial.
    virtual void reset() = 0;
};

#endif // IFURNACE_H