#include "Telemetry.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
Telemetry::Telemetry() : _lineCount(0) {
}

// ------------------------------------------------------------
// Inicializacao da serial
// ------------------------------------------------------------
void Telemetry::begin(uint32_t baudRate) {
    Serial.begin(baudRate);
    _lineCount = 0;
}

// ------------------------------------------------------------
// Emissao periodica
// ------------------------------------------------------------
void Telemetry::emit(uint32_t tsMs, KilnState state,
                     float tempC, float setpointC, float powerPct,
                     bool ssrOn, int8_t segment, uint32_t elapsedS,
                     FaultCode fault, AlarmState alarm) {

#if TELEMETRY_ENABLED
    char buf[192];
    size_t pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"ts\":%lu,\"st\":\"%s\",\"t\":",
        (unsigned long)tsMs, stateToString(state));

    appendFloat1(buf, sizeof(buf), pos, tempC);

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",\"sp\":");
    appendFloat1(buf, sizeof(buf), pos, setpointC);

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",\"p\":");
    appendFloat1(buf, sizeof(buf), pos, powerPct);

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        ",\"ssr\":%s,\"seg\":%d,\"el\":%lu,\"flt\":\"%s\",\"alm\":\"%s\"}\n",
        ssrOn ? "true" : "false",
        (int)segment,
        (unsigned long)elapsedS,
        faultToString(fault),
        alarmToString(alarm));

    Serial.print(buf);
    _lineCount++;
#endif
}

// ------------------------------------------------------------
// Emissao de evento
// ------------------------------------------------------------
void Telemetry::emitEvent(uint32_t tsMs, const char* event,
                          KilnState state, FaultCode fault) {

#if TELEMETRY_ENABLED
    char buf[128];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"ev\":\"%s\",\"st\":\"%s\",\"flt\":\"%s\"}\n",
        (unsigned long)tsMs,
        event ? event : "",
        stateToString(state),
        faultToString(fault));

    Serial.print(buf);
    _lineCount++;
#endif
}

// ------------------------------------------------------------
// Float com 1 decimal, sem depender de printf de double
// ------------------------------------------------------------
void Telemetry::appendFloat1(char* buf, size_t bufSize, size_t& pos, float value) {
    if (pos >= bufSize) {
        return;
    }

    // Trata NaN e infinito explicitamente
    if (isnan(value)) {
        pos += snprintf(buf + pos, bufSize - pos, "null");
        return;
    }
    if (isinf(value)) {
        pos += snprintf(buf + pos, bufSize - pos, "null");
        return;
    }

    // Sinal
    bool negative = (value < 0.0f);
    if (negative) {
        value = -value;
    }

    // Arredonda para 1 decimal
    uint32_t scaled = (uint32_t)(value * 10.0f + 0.5f);
    uint32_t whole  = scaled / 10;
    uint32_t frac   = scaled % 10;

    pos += snprintf(buf + pos, bufSize - pos, "%s%lu.%lu",
        negative ? "-" : "",
        (unsigned long)whole,
        (unsigned long)frac);
}