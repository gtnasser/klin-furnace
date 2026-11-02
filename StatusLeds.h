#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// StatusLeds - Sinalizacao visual de estado e falha
// V1: LED de run e LED de fault.
// LED de ack e buzzer reservados para versoes futuras.
//
// Opera em TEMPO REAL: LEDs sao sinalizacao para humano.
//
// Guarda de seguranca: pinos em PIN_UNASSIGNED nao recebem
// escrita. Permite que o controlador rode em EMERGENCY sem
// hardware inicializado.
// ============================================================

class StatusLeds {
public:
    StatusLeds();

    // Configura os pinos. Chamar uma vez no setup.
    // Pinos em PIN_UNASSIGNED ficam inoperantes.
    void begin(uint8_t runPin, uint8_t faultPin);

    // Atualiza os LEDs. Chamar a cada ciclo do loop.
    // state  : estado atual da maquina
    // fault  : falha ativa
    // dtMs   : delta de tempo REAL em milissegundos
    void update(KilnState state, FaultCode fault, uint32_t dtMs);

    // Desliga todos os LEDs.
    void allOff();

private:
    uint8_t  _runPin;
    uint8_t  _faultPin;
    bool     _runOn;
    bool     _faultOn;
    uint32_t _blinkTimerMs;
    bool     _blinkPhase;

    void setRun(bool on);
    void setFault(bool on);
};

#endif // STATUS_LEDS_H