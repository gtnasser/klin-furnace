#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// Telemetry - Emissao de telemetria em JSON por linha
// Um objeto JSON por linha, terminado em '\n'.
// Campos curtos, tempo em milissegundos SIMULADOS.
// ============================================================

class Telemetry {
public:
    Telemetry();

    // Configura a saida serial.
    void begin(uint32_t baudRate);

    // Emite uma linha de telemetria.
    // tsMs     : tempo SIMULADO desde o boot, em ms
    // state    : estado atual da maquina
    // tempC    : temperatura atual
    // setpointC: setpoint atual
    // powerPct : potencia do PID (0-100)
    // ssrOn    : estado binario do SSR
    // segment  : indice do segmento do perfil (0-based, -1 se n/a)
    // elapsedS : tempo decorrido no perfil, em segundos simulados
    // fault    : falha ativa
    // alarm    : estado do alarme
    void emit(uint32_t tsMs, KilnState state,
              float tempC, float setpointC, float powerPct,
              bool ssrOn, int8_t segment, uint32_t elapsedS,
              FaultCode fault, AlarmState alarm);

    // Emite uma linha de evento (nao periodica).
    // Usada para transicoes de estado e falhas.
    void emitEvent(uint32_t tsMs, const char* event,
                   KilnState state, FaultCode fault);

    // Contador de linhas emitidas (diagnostico).
    uint32_t getLineCount() const { return _lineCount; }

private:
    uint32_t _lineCount;

    // Formata float com 1 decimal sem usar printf de double
    void appendFloat1(char* buf, size_t bufSize, size_t& pos, float value);
};

#endif // TELEMETRY_H