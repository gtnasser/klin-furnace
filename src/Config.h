#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// Config.h - Configuracao central do firmware
//
// Convencao:
//   - #define para valores fixos (nao mudam em runtime)
//   - extern para dados definidos em Config.cpp (arrays de perfil)
//   - extern para as globais mutaveis (definidas no main.cpp)
//
// Pinagem auditada contra restricoes do ESP32:
//   - Nenhum pino de strapping (0, 2, 5, 12, 15) em uso
//   - Nenhum pino de flash SPI (6-11) em uso
//   - ADC2 e touch nao sao usados: restricoes inofensivas
//   - Leitura analogica confiavel usa ADC1 (34, 35, 36, 39)
// ============================================================

// ------------------------------------------------------------
// Limites termicos
// ------------------------------------------------------------
#define MAX_KILN_TEMP_C        1100.0f   // teto do perfil
#define SAFETY_LIMIT_C         1150.0f   // dispara OVER_TEMPERATURE
#define MIN_VALID_TEMP_C       (-50.0f)  // abaixo disso: leitura invalida
#define RESET_ALLOWED_C        200.0f    // reset so abaixo desta temp

// ------------------------------------------------------------
// Pinos - atuadores e sinalizacao (V1)
// ------------------------------------------------------------
#define SSR_PIN                25
#define LED_RUN_PIN            23    // GPIO 2 e strap de boot: evitado
#define LED_FAULT_PIN          4

// ------------------------------------------------------------
// Pinos - reservados para o V2 (nao usados no V1)
// ------------------------------------------------------------
#define LED_ACK_PIN            13    // GPIO 15 e strap de boot: evitado
#define BUZZER_PIN             16
#define RESET_BUTTON_PIN       32    // pull-down externo 10k, ativo em HIGH
#define ACK_BUTTON_PIN         33    // pull-down externo 10k, ativo em HIGH

// ------------------------------------------------------------
// Termopar MAX31855 (SPI) - definidos para o V2
// ------------------------------------------------------------
#define THERMO_CLK_PIN         18
#define THERMO_CS_PIN          17    // GPIO 5 e strap de boot: evitado
#define THERMO_DO_PIN          19

// ------------------------------------------------------------
// Barramento I2C - reservado para display OLED e/ou RTC (V2)
// ------------------------------------------------------------
#define I2C_SDA_PIN            21    // pino I2C padrao do ESP32
#define I2C_SCL_PIN            22    // pino I2C padrao do ESP32

// ------------------------------------------------------------
// Encoder rotativo - reservado para o V2
// Canal B no GPIO 14; botao no GPIO 26 (ADC2, digital ok).
// ------------------------------------------------------------
#define ENCODER_A_PIN          27
#define ENCODER_B_PIN          14
#define ENCODER_BTN_PIN        26    // digital ok; analogico so sem Wi-Fi

// ------------------------------------------------------------
// Entradas analogicas - ADC1 apenas (V3)
// ADC1 funciona independente do Wi-Fi; ADC2 nao.
// 34, 35, 36 e 39 sao SOMENTE ENTRADA e sem pull interno:
// exigem divisor resistivo ou sensor com saida propria.
//
// Dois papeis distintos, com requisitos eletricos opostos:
//   - tensao da rede: divisor + trafo de isolacao + protecao
//   - calibracao: referencia estavel (TL431), baixa tensao CC
// ------------------------------------------------------------
#define ANALOG_MAINS_V_PIN     34    // ADC1 ch6 - tensao da rede
#define ANALOG_CALIB_REF_PIN   35    // ADC1 ch7 - referencia de calibracao
#define ANALOG_SPARE_1_PIN     36    // ADC1 ch0 - reserva
#define ANALOG_SPARE_2_PIN     39    // ADC1 ch3 - reserva

// ------------------------------------------------------------
// SSR - modulacao por time-proportioning
// ------------------------------------------------------------
#define SSR_WINDOW_MS          10000     // janela de modulacao
#define SSR_MIN_ON_MS          100       // pulso minimo confiavel

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
#define STUCK_WINDOW_S         300.0f    // janela de confirmacao
#define STUCK_DELTA_THRESHOLD  0.5f      // variacao considerada "plana"
#define STUCK_POWER_THRESHOLD  20.0f     // potencia considerada "aquecendo"
#define STUCK_SETPOINT_BAND    20.0f     // erro minimo para "longe do alvo"

// ------------------------------------------------------------
// Serial e telemetria
// ------------------------------------------------------------
#define SERIAL_BAUD_RATE       115200
#define TELEMETRY_ENABLED      1
#define TELEMETRY_PERIOD_DEFAULT_S  3.0f

// ------------------------------------------------------------
// Escala de tempo
// ------------------------------------------------------------
#define TIME_SCALE_DEFAULT     1.0f

// ------------------------------------------------------------
// Limites dos comandos seriais (X<n> e P<n>)
// ------------------------------------------------------------
#define TIME_SCALE_MIN         1.0f
#define TIME_SCALE_MAX         500.0f
#define TELEMETRY_PERIOD_MIN   1.0f
#define TELEMETRY_PERIOD_MAX   600.0f

// ------------------------------------------------------------
// Tabela de perfis de queima
//
// ProfileId deixa de ser enum: passa a ser indice uint8_t na
// tabela ProfileEntry[]. Isso permite 10 perfis sem switch.
// Slots vazios tem points == nullptr e count == 0.
// ------------------------------------------------------------
#define PROFILE_COUNT          10
#define PROFILE_NONE           0xFF   // sentinela: nenhum perfil ativo

struct ProfileEntry {
    const char*        name;
    const float (*points)[2];
    uint8_t            count;
};

// Tabela definida em Config.cpp
extern const ProfileEntry PROFILE_TABLE[PROFILE_COUNT];

// Perfil ativo na inicializacao (0 = FAST_PROFILE)
#define PROFILE_DEFAULT        0

// ------------------------------------------------------------
// Globais mutaveis em runtime (definidas no main.cpp)
// ------------------------------------------------------------
extern float     g_timeScale;
extern float     g_telemetryPeriodS;
extern uint8_t   g_activeProfile;

#endif // CONFIG_H