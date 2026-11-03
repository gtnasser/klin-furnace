#include <Arduino.h>
#include "Config.h"
#include "Types.h"
#include "KilnProfile.h"
#include "FaultDetector.h"
#include "PIDController.h"
#include "SSRDriver.h"
#include "SimulatedFurnace.h"
#include "Telemetry.h"
#include "StatusLeds.h"
#include "KilnController.h"

// ============================================================
// main.cpp - Ponto de entrada do firmware
// V1: simulacao no Wokwi
//
// Comandos seriais:
//   S      - inicia o perfil ativo
//   R      - solicita reset apos falha/fim
//   T      - alterna entre PRODUCTION e TEST (apenas em IDLE)
//   I      - imprime o estado completo atual
//   X<n>   - ajusta TIME_SCALE para n
//   P<n>   - ajusta periodo de telemetria para n segundos simulados
//   ?      - mostra ajuda
// ============================================================

// ------------------------------------------------------------
// Globais mutaveis (declaradas em Config.h)
// ------------------------------------------------------------
float     g_timeScale        = TIME_SCALE_DEFAULT;
float     g_telemetryPeriodS = TELEMETRY_PERIOD_DEFAULT_S;
ProfileId g_activeProfile    = PROFILE_DEFAULT;

// ------------------------------------------------------------
// Objetos globais
// ------------------------------------------------------------
SimulatedFurnace furnace;      // V2: trocar por RealFurnace
SSRDriver        ssr;
PIDController    pid;
FaultDetector    detector;
Telemetry        telemetry;
StatusLeds       leds;

KilnController controller(&furnace, &ssr, &pid,
                          &detector, &telemetry, &leds);

// ------------------------------------------------------------
// Mapa de pinos (inicializado com PIN_UNASSIGNED)
// ------------------------------------------------------------
static HardwarePins PINS = {
    PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED,
    PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED
};

static uint32_t lastLoopMs = 0;

// ------------------------------------------------------------
// Selecao de perfil
// ------------------------------------------------------------
static const float (*selectProfile())[2] {
    return (g_activeProfile == PROFILE_TEST) ? TEST_POINTS
                                             : PRODUCTION_POINTS;
}

static uint8_t selectProfileLength() {
    return (g_activeProfile == PROFILE_TEST) ? TEST_POINTS_LEN
                                             : PRODUCTION_POINTS_LEN;
}

static const char* profileName() {
    return (g_activeProfile == PROFILE_TEST) ? "TEST" : "PRODUCTION";
}

// ------------------------------------------------------------
// Comandos seriais
// ------------------------------------------------------------
static void printHelp() {
    Serial.println("{\"ev\":\"HELP\",\"cmds\":\"S=start R=reset T=toggle I=info X<n>=timeScale P<n>=telemPeriod ?=help\"}");
}

static void cmdStart() {
    const ProfileStatus st = controller.start(selectProfile(),
                                              selectProfileLength());
    if (st == ProfileStatus::NOT_IDLE) {
        Serial.println("{\"ev\":\"START_DENIED\",\"reason\":\"NOT_IDLE\"}");
        return;
    }
    if (st != ProfileStatus::OK) {
        Serial.print("{\"ev\":\"PROFILE_INVALID\",\"reason\":\"");
        Serial.print(profileStatusToString(st));
        Serial.println("\"}");
        return;
    }
    Serial.print("{\"ev\":\"START\",\"profile\":\"");
    Serial.print(profileName());
    Serial.println("\"}");
}

static void cmdReset() {
    if (controller.requestReset()) {
        Serial.println("{\"ev\":\"RESET_OK\"}");
        return;
    }
    const KilnState s = controller.getState();
    if (s != KilnState::EMERGENCY && s != KilnState::FINISHED) {
        Serial.println("{\"ev\":\"RESET_DENIED\",\"reason\":\"NOT_ALLOWED_IN_STATE\"}");
    } else {
        Serial.println("{\"ev\":\"RESET_DENIED\",\"reason\":\"TOO_HOT\"}");
    }
}

static void cmdToggleProfile() {
    if (controller.getState() != KilnState::IDLE) {
        Serial.println("{\"ev\":\"PROFILE_CHANGED_DENIED\",\"reason\":\"NOT_IDLE\"}");
        return;
    }

    g_activeProfile = (g_activeProfile == PROFILE_TEST)
                      ? PROFILE_PRODUCTION
                      : PROFILE_TEST;

    Serial.print("{\"ev\":\"PROFILE_CHANGED\",\"profile\":\"");
    Serial.print(profileName());
    Serial.println("\"}");
}

static void cmdInfo() {
    Serial.print("{\"ev\":\"INFO\",\"st\":\"");
    Serial.print(stateToString(controller.getState()));
    Serial.print("\",\"flt\":\"");
    Serial.print(faultToString(controller.getFault()));
    Serial.print("\",\"alm\":\"");
    Serial.print(alarmToString(controller.getAlarm()));
    Serial.print("\",\"t\":");
    Serial.print(controller.getTemperature(), 1);
    Serial.print(",\"sp\":");
    Serial.print(controller.getSetpoint(), 1);
    Serial.print(",\"p\":");
    Serial.print(controller.getPower(), 1);
    Serial.print(",\"el\":");
    Serial.print(controller.getElapsedSimS(), 0);
    Serial.print(",\"seg\":");
    Serial.print((int)controller.getSegment());
    Serial.print(",\"profile\":\"");
    Serial.print(profileName());
    Serial.print("\",\"tscale\":");
    Serial.print(g_timeScale, 1);
    Serial.print(",\"tperiod\":");
    Serial.print(g_telemetryPeriodS, 1);
    Serial.println("}");
}

static void cmdSetTimeScale(float value) {
    if (value < TIME_SCALE_MIN || value > TIME_SCALE_MAX) {
        Serial.print("{\"ev\":\"TIMESCALE_DENIED\",\"reason\":\"OUT_OF_RANGE\",\"min\":");
        Serial.print(TIME_SCALE_MIN, 1);
        Serial.print(",\"max\":");
        Serial.print(TIME_SCALE_MAX, 1);
        Serial.println("}");
        return;
    }
    g_timeScale = value;
    Serial.print("{\"ev\":\"TIMESCALE_SET\",\"value\":");
    Serial.print(g_timeScale, 1);
    Serial.println("}");
}

static void cmdSetTelemetryPeriod(float value) {
    if (value < TELEMETRY_PERIOD_MIN || value > TELEMETRY_PERIOD_MAX) {
        Serial.print("{\"ev\":\"TELEMPERIOD_DENIED\",\"reason\":\"OUT_OF_RANGE\",\"min\":");
        Serial.print(TELEMETRY_PERIOD_MIN, 1);
        Serial.print(",\"max\":");
        Serial.print(TELEMETRY_PERIOD_MAX, 1);
        Serial.println("}");
        return;
    }
    g_telemetryPeriodS = value;
    Serial.print("{\"ev\":\"TELEMPERIOD_SET\",\"value\":");
    Serial.print(g_telemetryPeriodS, 1);
    Serial.println("}");
}

// ------------------------------------------------------------
// Leitura de numero apos o prefixo do comando
// Consome digitos e ponto decimal ate encontrar terminador.
// Retorna false se nenhum digito valido foi lido.
// ------------------------------------------------------------
static bool readNumericArg(float& outValue) {
    char buf[12];
    uint8_t len = 0;
    bool hasDigit = false;

    while (Serial.available() > 0 && len < sizeof(buf) - 1) {
        const char c = (char)Serial.peek();
        if ((c >= '0' && c <= '9') || c == '.') {
            buf[len++] = (char)Serial.read();
            if (c != '.') hasDigit = true;
        } else {
            break;
        }
    }
    buf[len] = '\0';

    if (!hasDigit) {
        return false;
    }
    outValue = atof(buf);
    return true;
}

static void handleSerialCommands() {
    while (Serial.available() > 0) {
        const char c = (char)Serial.read();

        switch (c) {
            case 'S': case 's':
                cmdStart();
                break;

            case 'R': case 'r':
                cmdReset();
                break;

            case 'T': case 't':
                cmdToggleProfile();
                break;

            case 'I': case 'i':
                cmdInfo();
                break;

            case 'X': case 'x': {
                float v = 0.0f;
                if (readNumericArg(v)) {
                    cmdSetTimeScale(v);
                } else {
                    Serial.println("{\"ev\":\"TIMESCALE_DENIED\",\"reason\":\"MISSING_VALUE\"}");
                }
                break;
            }

            case 'P': case 'p': {
                float v = 0.0f;
                if (readNumericArg(v)) {
                    cmdSetTelemetryPeriod(v);
                } else {
                    Serial.println("{\"ev\":\"TELEMPERIOD_DENIED\",\"reason\":\"MISSING_VALUE\"}");
                }
                break;
            }

            case '?':
                printHelp();
                break;

            case '\n': case '\r': case ' ':
                break;

            default:
                Serial.println("{\"ev\":\"UNKNOWN_CMD\"}");
                break;
        }
    }
}

// ------------------------------------------------------------
// setup()
// ------------------------------------------------------------
void setup() {
    telemetry.begin(SERIAL_BAUD_RATE);

    // Mapa de pinos: atribuicao campo a campo (C++11)
    PINS.ssr         = SSR_PIN;
    PINS.ledRun      = LED_RUN_PIN;
    PINS.ledFault    = LED_FAULT_PIN;
    PINS.ledAck      = LED_ACK_PIN;
    PINS.buzzer      = BUZZER_PIN;
    PINS.buttonReset = RESET_BUTTON_PIN;
    PINS.buttonAck   = ACK_BUTTON_PIN;

    if (!controller.begin(PINS)) {
        Serial.println("{\"ev\":\"PIN_CONFIG_INVALID\",\"state\":\"EMERGENCY\"}");
        lastLoopMs = millis();
        return;
    }

    Serial.print("{\"ev\":\"READY\",\"profile\":\"");
    Serial.print(profileName());
    Serial.print("\",\"timeScale\":");
    Serial.print(g_timeScale, 1);
    Serial.print(",\"telemPeriod\":");
    Serial.print(g_telemetryPeriodS, 1);
    Serial.println("}");
    printHelp();

    lastLoopMs = millis();
}

// ------------------------------------------------------------
// loop()
// ------------------------------------------------------------
void loop() {
    const uint32_t now = millis();
    const uint32_t dtMs = now - lastLoopMs;
    lastLoopMs = now;

    if (dtMs > 5000) {
        return;
    }

    handleSerialCommands();
    controller.update(dtMs);
}

