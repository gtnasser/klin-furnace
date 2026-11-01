#ifndef SSR_DRIVER_H
#define SSR_DRIVER_H

#include <Arduino.h>

// ============================================================
// SSRDriver - Acionamento do SSR por time-proportioning
// Converte potencia (0-100%) em pulsos dentro de uma janela fixa.
// Ex.: 30% em janela de 10 s = 3 s ligado / 7 s desligado.
// ============================================================

class SSRDriver {
public:
    SSRDriver();

    // Configura o pino e o estado inicial (desligado).
    void begin(uint8_t pin);

    // Atualiza a modulacao. Chamar a cada ciclo do loop.
    // power    : potencia desejada 0-100%
    // dtMs     : delta de tempo REAL em milissegundos
    //            (o SSR opera em tempo real, nao simulado)
    void update(float power, uint32_t dtMs);

    // Desliga imediatamente e trava a saida em OFF.
    // Usado em EMERGENCY.
    void forceOff();

    // Libera o travamento e volta a operar normalmente.
    // DEVE ser chamado ao sair de EMERGENCY (bug corrigido).
    void reset();

    // Estado fisico atual do pino.
    bool isOn() const { return _isOn; }

    // Potencia aplicada no ultimo update.
    float getPower() const { return _power; }

private:
    uint8_t  _pin;
    float    _power;        // potencia alvo (0-100)
    bool     _isOn;         // estado atual do pino
    bool     _forcedOff;    // travamento por emergencia
    uint32_t _windowElapsedMs;  // tempo decorrido na janela atual

    void setOutput(bool on);
};

#endif // SSR_DRIVER_H