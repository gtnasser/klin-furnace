#ifndef IFURNACE_H
#define IFURNACE_H

#include <Arduino.h>

// ============================================================
// IFurnace - Interface do forno
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

    virtual void  begin() = 0;
    virtual void  setPower(float powerPct, float dtSim) = 0;
    virtual float getTemperature() = 0;
    virtual void  reset() = 0;
};

#endif // IFURNACE_H