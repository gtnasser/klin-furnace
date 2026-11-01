#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================================
// Types.h - Tipos compartilhados
// ============================================================

// ------------------------------------------------------------
// Estados da maquina de estados do controlador
// ------------------------------------------------------------
enum class KilnState : uint8_t {
    IDLE = 0,       // Aguardando comando de inicio
    STARTING,       // Validando perfil e condicoes iniciais
    RUNNING,        // Executando o perfil de queima
    FINISHED,       // Perfil concluido normalmente
    RESETTING,      // Retornando ao estado seguro
    EMERGENCY       // Falha critica: SSR desligado, aguardando reset
};

// ------------------------------------------------------------
// Codigos de falha
// ------------------------------------------------------------
enum class FaultCode : uint8_t {
    NONE = 0,           // Sem falha ativa
    THERMOCOUPLE_OPEN,  // Leitura invalida (NaN ou fora de faixa)
    SENSOR_STUCK,       // Sensor sem variacao com forno aquecendo
    OVER_TEMPERATURE,   // Acima de SAFETY_LIMIT_C
    SSR_STUCK_ON,       // STUB V1: declarado, NAO gerado (requer realimentacao fisica do SSR - V2)
    HEATING_FAILURE,    // STUB V1: declarado, NAO gerado (requer leitura real de temperatura - V2)
};

// ------------------------------------------------------------
// Estado do alarme
// ------------------------------------------------------------
enum class AlarmState : uint8_t {
    NO_ALARM = 0,       // Nenhum alarme ativo
    ACTIVE_UNACK,       // Alarme ativo, aguardando reconhecimento
    ACTIVE_ACK          // Alarme ativo, ja reconhecido
};

// ------------------------------------------------------------
// Utilitarios de conversao para telemetria
// ------------------------------------------------------------
inline const char* stateToString(KilnState s) {
    switch (s) {
        case KilnState::IDLE:      return "IDLE";
        case KilnState::STARTING:  return "STARTING";
        case KilnState::RUNNING:   return "RUNNING";
        case KilnState::FINISHED:  return "FINISHED";
        case KilnState::RESETTING: return "RESETTING";
        case KilnState::EMERGENCY: return "EMERGENCY";
        default:                   return "UNKNOWN";
    }
}

inline const char* faultToString(FaultCode f) {
    switch (f) {
        case FaultCode::NONE:              return "NONE";
        case FaultCode::THERMOCOUPLE_OPEN: return "THERMOCOUPLE_OPEN";
        case FaultCode::SENSOR_STUCK:      return "SENSOR_STUCK";
        case FaultCode::OVER_TEMPERATURE:  return "OVER_TEMPERATURE";
        case FaultCode::SSR_STUCK_ON:      return "SSR_STUCK_ON";
        case FaultCode::HEATING_FAILURE:   return "HEATING_FAILURE";
        case FaultCode::INVALID_PROFILE:   return "INVALID_PROFILE";
        default:                           return "UNKNOWN";
    }
}

inline const char* alarmToString(AlarmState a) {
    switch (a) {
        case AlarmState::NO_ALARM:     return "NONE";
        case AlarmState::ACTIVE_UNACK: return "ACTIVE_UNACK";
        case AlarmState::ACTIVE_ACK:   return "ACTIVE_ACK";
        default:                       return "UNKNOWN";
    }
}

#endif // TYPES_H