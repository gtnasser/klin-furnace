#ifndef SSR_DRIVER_H
#define SSR_DRIVER_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// SSRDriver - Modulacao do SSR por time-proportioning
//
// Opera em TEMPO REAL: a janela de modulacao e contada em ms
// reais, independente da escala de tempo da simulacao termica.
//
// Guarda de seguranca: se _pin == PIN_UNASSIGNED, nenhuma
// escrita e feita no GPIO. Isso evita escrever no GPIO 0
// (pino de boot do ESP32) quando o hardware nao foi inicializado.
// ============================================================

class SSRDriver {
public:
    SSRDriver();

    // Configura o pino e o estado inicial (desligado).
    // Se pin == PIN_UNASSIGNED, o driver fica inoperante.
    void begin(uint8_t pin);

    // Atualiza a modulacao. Chamar a cada ciclo do loop.
    // powerPct : potencia desejada (0-100)
    // dtRealMs : delta de tempo REAL em milissegundos
    void update(float powerPct, uint32_t dtRealMs);

    // Forca desligamento imediato e trava a saida.
    // Usado em falhas: nenhuma potencia e aplicada ate reset().
    void forceOff();

    // Libera o travamento e zera o estado.
    void reset();

    bool  isOn() const { return _isOn; }
    bool  isForcedOff() const { return _forcedOff; }
    float getPower() const { return _power; }

private:
    uint8_t  _pin;
    float    _power;
    bool     _isOn;
    bool     _forcedOff;
    uint32_t _windowElapsedMs;

    void setOutput(bool on);
};

#endif // SSR_DRIVER_H