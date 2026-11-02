#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// Config.h - Configuracao central do firmware
//
// Convencao:
//   - #define para valores fixos (nao mudam em runtime)
//   - extern para dados definidos em Config.cpp (arrays de perfil)
//   - extern para as tres globais mutaveis (definidas no main.cpp)
// ============================================================

// ------------------------------------------------------------
// Limites termicos
// ------------------------------------------------------------
#define MAX_KILN_TEMP_C        1100.0f
#define SAFETY_LIMIT_C         1150.0f
#define MIN_VALID_TEMP_C       (-50.0f)
#define RESET_ALLOWED_C        200.0f

// ------------------------------------------------------------
// Pinos - atuadores e sinalizacao (V1)
// ------------------------------------------------------------
#define SSR_PIN                25
#define LED_RUN_PIN            2
#define LED_FAULT_PIN          4

// ------------------------------------------------------------
// Pinos - reservados para o V2
// ------------------------------------------------------------
#define LED_ACK_PIN            15
#define BUZZER_PIN             16
#define RESET_BUTTON_PIN       32
#define ACK_BUTTON_PIN         33

#define THERMO_CLK_PIN         18
#define THERMO_CS_PIN          5
#define THERMO_DO_PIN          19

// ------------------------------------------------------------
// SSR
// ------------------------------------------------------------
#define SSR_WINDOW_MS          10000
#define SSR_MIN_ON_MS          100

// ------------------------------------------------------------
// PID
// ------------------------------------------------------------
#define PID_KP                 2.5f
#define PID_KI                 0.02f
#define PID_KD                 15.0f
#define PID_OUTPUT_MIN         0.0f
#define PID_OUTPUT_MAX         100.0f
#define PID_INTEGRAL_MIN       (-50.0f)
#define PID_INTEGRAL_MAX       100.0f

// ------------------------------------------------------------
// Deteccao de falhas
// ------------------------------------------------------------
#define STUCK_WINDOW_S         300.0f
#define STUCK_DELTA_THRESHOLD  0.5f
#define STUCK_POWER_THRESHOLD  20.0f
#define STUCK_SETPOINT_BAND    20.0f

// ------------------------------------------------------------
// Serial e telemetria
// ------------------------------------------------------------
#define SERIAL_BAUD_RATE       115200
#define TELEMETRY_ENABLED      1
#define TELEMETRY_PERIOD_DEFAULT_S  5.0f

// ------------------------------------------------------------
// Escala de tempo
// ------------------------------------------------------------
#define TIME_SCALE_DEFAULT     50.0f

// ------------------------------------------------------------
// Limites dos comandos seriais (X<n> e P<n>)
// ------------------------------------------------------------
#define TIME_SCALE_MIN         1.0f
#define TIME_SCALE_MAX         500.0f
#define TELEMETRY_PERIOD_MIN   1.0f
#define TELEMETRY_PERIOD_MAX   600.0f

// ------------------------------------------------------------
// Identificador do perfil ativo
// ------------------------------------------------------------
enum ProfileId : uint8_t {
    PROFILE_PRODUCTION = 0,
    PROFILE_TEST       = 1
};

// ------------------------------------------------------------
// Perfis de queima (definidos em Config.cpp)
// ------------------------------------------------------------
extern const float PRODUCTION_POINTS[][2];
extern const uint8_t PRODUCTION_POINTS_LEN;

extern const float TEST_POINTS[][2];
extern const uint8_t TEST_POINTS_LEN;

// ------------------------------------------------------------
// Perfil ativo na inicializacao
// ------------------------------------------------------------
#define PROFILE_DEFAULT        PROFILE_TEST

// ------------------------------------------------------------
// Globais mutaveis em runtime (definidas no main.cpp)
// ------------------------------------------------------------
extern float     g_timeScale;
extern float     g_telemetryPeriodS;
extern ProfileId g_activeProfile;

#endif // CONFIG_H