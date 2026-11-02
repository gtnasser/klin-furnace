#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// Telemetry - Saida JSON por linha no monitor serial
//
// Formato: um objeto JSON por linha, terminado em \n.
// Dois tipos de mensagem:
//   - Evento   : {"ts":..,"ev":"..","st":"..","flt":".."}
//   - Periodica: {"ts":..,"st":..,"t":..,"sp":..,"p":..,...}
//
// Tempo em SIMULADO: ts vem do KilnController.
// ============================================================

class Telemetry {
public:
    Telemetry();

    // Configura a serial. Chamar uma vez no setup.
    void begin(uint32_t baudRate);

    // Emite um evento discreto (transicao de estado, falha, comando).
    void emitEvent(uint32_t tsSimMs, const char* event,
                   KilnState state, FaultCode fault);

    // Emite uma linha periodica de grandezas.
    void emit(uint32_t tsSimMs,
              KilnState state,
              float temperature,
              float setpoint,
              float power,
              bool ssrOn,
              int8_t segment,
              uint32_t elapsedS,
              FaultCode fault,
              AlarmState alarm);

private:
    bool _enabled;

    void printFloat1(float value);
    void printFloat0(float value);
};

#endif // TELEMETRY_H