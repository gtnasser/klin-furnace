#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================================
// Types.h - Enums, structs e conversoes para texto
// ============================================================

// ------------------------------------------------------------
// Valor sentinela para pino nao configurado.
// 0xFF nao corresponde a nenhum GPIO valido do ESP32.
// ------------------------------------------------------------
#define PIN_UNASSIGNED  0xFF

// ------------------------------------------------------------
// Estado da maquina
// ------------------------------------------------------------
enum class KilnState : uint8_t {
    IDLE       = 0,
    STARTING   = 1,
    RUNNING    = 2,
    FINISHED   = 3,
    RESETTING  = 4,
    EMERGENCY  = 5
};

inline const char* stateToString(KilnState s) {
    switch (s) {
        case KilnState::IDLE:      return "IDLE";
        case KilnState::STARTING:  return "STARTING";
        case KilnState::RUNNING:   return "RUNNING";
        case KilnState::FINISHED:  return "FINISHED";
        case KilnState::RESETTING: return "RESETTING";
        case KilnState::EMERGENCY: return "EMERGENCY";
    }
    return "UNKNOWN";
}

// ------------------------------------------------------------
// Codigo de falha
// ------------------------------------------------------------
enum class FaultCode : uint8_t {
    NONE              = 0,
    THERMOCOUPLE_OPEN = 1,
    SENSOR_STUCK      = 2,
    OVER_TEMPERATURE  = 3,
    SSR_STUCK_ON      = 4,   // V2
    HEATING_FAILURE   = 5    // V2
};

inline const char* faultToString(FaultCode f) {
    switch (f) {
        case FaultCode::NONE:              return "NONE";
        case FaultCode::THERMOCOUPLE_OPEN: return "THERMOCOUPLE_OPEN";
        case FaultCode::SENSOR_STUCK:      return "SENSOR_STUCK";
        case FaultCode::OVER_TEMPERATURE:  return "OVER_TEMPERATURE";
        case FaultCode::SSR_STUCK_ON:      return "SSR_STUCK_ON";
        case FaultCode::HEATING_FAILURE:   return "HEATING_FAILURE";
    }
    return "UNKNOWN";
}

// ------------------------------------------------------------
// Estado do alarme
// ------------------------------------------------------------
enum class AlarmState : uint8_t {
    NO_ALARM      = 0,
    ACTIVE_UNACK  = 1,
    ACTIVE_ACK    = 2,   // V2
    CLEARED       = 3
};

inline const char* alarmToString(AlarmState a) {
    switch (a) {
        case AlarmState::NO_ALARM:     return "NONE";
        case AlarmState::ACTIVE_UNACK: return "ACTIVE_UNACK";
        case AlarmState::ACTIVE_ACK:   return "ACTIVE_ACK";
        case AlarmState::CLEARED:      return "CLEARED";
    }
    return "UNKNOWN";
}

// ------------------------------------------------------------
// Status de validacao de perfil
// ------------------------------------------------------------
enum class ProfileStatus : uint8_t {
    OK                   = 0,
    TOO_FEW_POINTS       = 1,
    FIRST_POINT_NOT_ZERO = 2,
    TIME_NOT_INCREASING  = 3,
    TEMP_DECREASING      = 4,
    TEMP_OUT_OF_RANGE    = 5,
    DURATION_TOO_SHORT   = 6,
    NOT_IDLE             = 7
};

inline const char* profileStatusToString(ProfileStatus s) {
    switch (s) {
        case ProfileStatus::OK:                   return "OK";
        case ProfileStatus::TOO_FEW_POINTS:       return "TOO_FEW_POINTS";
        case ProfileStatus::FIRST_POINT_NOT_ZERO: return "FIRST_POINT_NOT_ZERO";
        case ProfileStatus::TIME_NOT_INCREASING:  return "TIME_NOT_INCREASING";
        case ProfileStatus::TEMP_DECREASING:      return "TEMP_DECREASING";
        case ProfileStatus::TEMP_OUT_OF_RANGE:    return "TEMP_OUT_OF_RANGE";
        case ProfileStatus::DURATION_TOO_SHORT:   return "DURATION_TOO_SHORT";
        case ProfileStatus::NOT_IDLE:             return "NOT_IDLE";
    }
    return "UNKNOWN";
}

// ------------------------------------------------------------
// Mapa de pinos do hardware
// Inicializar sempre com PIN_UNASSIGNED.
// ------------------------------------------------------------
struct HardwarePins {
    uint8_t ssr;
    uint8_t ledRun;
    uint8_t ledFault;
    uint8_t ledAck;        // V2
    uint8_t buzzer;        // V2
    uint8_t buttonReset;   // V2
    uint8_t buttonAck;     // V2
};

#endif // TYPES_H