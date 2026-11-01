#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// Config.h - Constantes e configuracao global
// Forno eletrico controlado por ESP32
// V1: simulacao (Wokwi) - aquecimento apenas
// ============================================================

// ------------------------------------------------------------
// 1. PINOS DE HARDWARE
// ------------------------------------------------------------
#define SSR_PIN             25   // Saida para o SSR (time-proportioning)

// Termopar: MAX31855 (SPI). NAO usado no V1 - reservado para V2.
// MAX31855 escolhido por ter deteccao de falha embutida
// (termopar aberto, curto, falha interna), o que reduz a
// dependencia do SENSOR_STUCK por software.
#define THERMOCOUPLE_CS     5    // Chip select (V2)
#define THERMOCOUPLE_SCK    18   // Clock SPI (V2)
#define THERMOCOUPLE_SO     19   // MISO SPI (V2)

#define LED_RUN_PIN         2    // LED verde: forno em operacao
#define LED_FAULT_PIN       4    // LED vermelho: falha ativa
#define LED_ACK_PIN         15   // LED amarelo: alarme reconhecido / mute
#define BUZZER_PIN          16   // Buzzer (nao usado no V1)

#define RESET_BUTTON_PIN    32   // Botao de reset (nao usado no V1)
#define ACK_BUTTON_PIN      33   // Botao de ack de alarme (nao usado no V1)

// ------------------------------------------------------------
// 2. LIMITES DE SEGURANCA
// ------------------------------------------------------------
#define TEMP_RESOLUTION_C   1.0f     // Resolucao adotada para temperatura
#define MAX_KILN_TEMP_C     1100.0f  // Limite maximo do perfil
#define SAFETY_LIMIT_C      1150.0f  // Dispara falha critica
#define MIN_VALID_TEMP_C    -20.0f   // Abaixo disso: sensor invalido
#define RESET_ALLOWED_C     200.0f   // Reset so permitido abaixo desta temp

// ------------------------------------------------------------
// 3. SSR - TIME PROPORTIONING
// ------------------------------------------------------------
#define SSR_WINDOW_MS       10000    // Janela de modulacao: 10 s
#define SSR_MIN_ON_MS       100      // Pulso minimo para nao queimar o SSR

// ------------------------------------------------------------
// 4. PID
// ------------------------------------------------------------
#define PID_KP              2.5f
#define PID_KI              0.02f
#define PID_KD              15.0f
#define PID_OUTPUT_MIN      0.0f
#define PID_OUTPUT_MAX      100.0f
#define PID_INTEGRAL_MIN    -50.0f
#define PID_INTEGRAL_MAX    50.0f

// ------------------------------------------------------------
// 5. DETECCAO DE FALHAS
// ------------------------------------------------------------
#define STUCK_POWER_THRESHOLD   60.0f   // % minima para considerar "aquecendo"
#define STUCK_DELTA_THRESHOLD   1.0f    // degC - resolucao do sistema
#define STUCK_SETPOINT_BAND     20.0f   // degC - acima disso esta "longe do alvo"
#define STUCK_WINDOW_S          300.0f  // s - janela de confirmacao

// ATENCAO: HEATING_FAILURE ainda NAO esta implementado no V1.
// As constantes abaixo ficam reservadas para o V2, quando houver
// leitura real de temperatura e for possivel medir taxa de subida.
#define HEATING_FAILURE_WINDOW_S 600.0f // s sem subir com potencia maxima
#define HEATING_FAILURE_MIN_RISE 5.0f   // degC minimos esperados na janela

// ------------------------------------------------------------
// 6. CONFIGURACAO DE RUNTIME (mutavel)
// ------------------------------------------------------------
// Valores padrao por compilacao. As variaveis globais sao
// definidas uma unica vez no main.cpp e podem ser alteradas
// em runtime (V2: comandos seriais).

#define TIME_SCALE_DEFAULT          50.0f   // Aceleracao do tempo simulado
#define TELEMETRY_PERIOD_DEFAULT_S  5.0f    // Periodo de telemetria (tempo simulado)

// Identificador do perfil ativo
enum ProfileId : uint8_t {
    PROFILE_PRODUCTION = 0,
    PROFILE_TEST       = 1
};

// Desenvolvimento: TEST. Trocar para PRODUCTION na validacao final.
#define PROFILE_DEFAULT  PROFILE_TEST


// Variaveis globais mutaveis (definidas no main.cpp)
extern float     g_timeScale;         // fator de aceleracao do tempo
extern float     g_telemetryPeriodS;  // periodo de telemetria em segundos simulados
extern ProfileId g_activeProfile;     // perfil selecionado

// ------------------------------------------------------------
// 7. PERFIL DE QUEIMA - PRODUCAO
// ------------------------------------------------------------
// 435 min (~7h15), monotonicamente crescente.
// Formato: { tempo_min, temperatura_C }
// ATENCAO: o ultimo ponto define o patamar final.
#define PROFILE_PRODUCTION_LEN  4

const float PROFILE_PRODUCTION[PROFILE_PRODUCTION_LEN][2] = {
    {   0.0f,  25.0f },  // P0 - ambiente
    { 180.0f, 300.0f },  // P1 - aquecimento 1 (1.53 degC/min)
    { 420.0f, 900.0f },  // P2 - aquecimento 2 (2.50 degC/min)
    { 435.0f, 900.0f }   // P3 - patamar (15 min)
};

// ------------------------------------------------------------
// 8. PERFIL DE QUEIMA - TESTE (simulacao rapida)
// ------------------------------------------------------------
// 10 min totais, rampas agressivas. Valida logica, nao termodinamica.
#define PROFILE_TEST_LEN  4

const float PROFILE_TEST[PROFILE_TEST_LEN][2] = {
    {  0.0f,  25.0f },
    {  3.0f, 300.0f },
    {  8.0f, 900.0f },
    { 10.0f, 900.0f }
};

// ------------------------------------------------------------
// 9. TELEMETRIA
// ------------------------------------------------------------
#define SERIAL_BAUD_RATE    115200
#define TELEMETRY_ENABLED   1

#endif // CONFIG_H