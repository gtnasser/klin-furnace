#include "Telemetry.h"
#include "Config.h"
#include <math.h>

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
Telemetry::Telemetry() : _enabled(false) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void Telemetry::begin(uint32_t baudRate) {
    Serial.begin(baudRate);
    _enabled = (TELEMETRY_ENABLED != 0);
}

// ------------------------------------------------------------
// Impressao de float com 1 casa decimal
// Serial.print(float, 1) nao existe em todas as versoes do
// core; impressao manual garante formato estavel.
// ------------------------------------------------------------
void Telemetry::printFloat1(float value) {
    if (isnan(value) || isinf(value)) {
        Serial.print("null");
        return;
    }
    Serial.print(value, 1);
}

void Telemetry::printFloat0(float value) {
    if (isnan(value) || isinf(value)) {
        Serial.print("null");
        return;
    }
    Serial.print(value, 0);
}

// ------------------------------------------------------------
// Evento discreto
// ------------------------------------------------------------
void Telemetry::emitEvent(uint32_t tsSimMs, const char* event,
                          KilnState state, FaultCode fault) {
    if (!_enabled) {
        return;
    }

    Serial.print("{\"ts\":");
    Serial.print(tsSimMs);
    Serial.print(",\"ev\":\"");
    Serial.print(event);
    Serial.print("\",\"st\":\"");
    Serial.print(stateToString(state));
    Serial.print("\",\"flt\":\"");
    Serial.print(faultToString(fault));
    Serial.println("\"}");
}

// ------------------------------------------------------------
// Linha periodica
// ------------------------------------------------------------
void Telemetry::emit(uint32_t tsSimMs,
                     KilnState state,
                     float temperature,
                     float setpoint,
                     float power,
                     bool ssrOn,
                     int8_t segment,
                     uint32_t elapsedS,
                     FaultCode fault,
                     AlarmState alarm) {
    if (!_enabled) {
        return;
    }

    Serial.print("{\"ts\":");
    Serial.print(tsSimMs);

    Serial.print(",\"st\":\"");
    Serial.print(stateToString(state));

    Serial.print("\",\"t\":");
    printFloat1(temperature);

    Serial.print(",\"sp\":");
    printFloat1(setpoint);

    Serial.print(",\"p\":");
    printFloat1(power);

    Serial.print(",\"ssr\":");
    Serial.print(ssrOn ? "true" : "false");

    Serial.print(",\"seg\":");
    Serial.print((int)segment);

    Serial.print(",\"el\":");
    Serial.print(elapsedS);

    Serial.print(",\"flt\":\"");
    Serial.print(faultToString(fault));

    Serial.print("\",\"alm\":\"");
    Serial.print(alarmToString(alarm));

    Serial.println("\"}");
}