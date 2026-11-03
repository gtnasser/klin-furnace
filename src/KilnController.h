#ifndef KILN_CONTROLLER_H
#define KILN_CONTROLLER_H

#include <Arduino.h>
#include "Types.h"
#include "KilnProfile.h"
#include "FaultDetector.h"
#include "PIDController.h"
#include "SSRDriver.h"
#include "IFurnace.h"
#include "Telemetry.h"
#include "StatusLeds.h"

// ============================================================
// KilnController - Orquestrador do forno
//
// Aplica escala de tempo, executa o perfil, coordena PID,
// deteccao de falhas, atuador, telemetria e sinalizacao.
//
// Assimetria de tempo (intencional):
//   - Modelo termico, PID e FaultDetector: tempo SIMULADO
//   - SSRDriver e StatusLeds: tempo REAL
// ============================================================

class KilnController {
public:
    KilnController(IFurnace* furnace,
                   SSRDriver* ssr,
                   PIDController* pid,
                   FaultDetector* detector,
                   Telemetry* telemetry,
                   StatusLeds* leds);

    // Inicializa todos os componentes. Chamar uma vez no setup.
    // Retorna false se o mapa de pinos contiver valor invalido
    // (PIN_UNASSIGNED), pino fora da faixa ou duplicado.
    // Em caso de falha, o controlador fica em EMERGENCY e
    // nenhum hardware e inicializado.
    bool begin(const HardwarePins& pins);

    // Ciclo principal. Chamar a cada iteracao do loop.
    // dtRealMs : delta de tempo REAL em milissegundos
    void update(uint32_t dtRealMs);

    // Solicita inicio do perfil. Retorna o status da validacao.
    // Retorna OK apenas se o perfil foi aceito e o estado
    // passou para STARTING.
    ProfileStatus start(const float (*points)[2], uint8_t count);

    // Solicita reset apos falha. So permitido abaixo de RESET_ALLOWED_C.
    bool requestReset();

    // Estado atual
    KilnState  getState() const { return _state; }
    FaultCode  getFault() const { return _fault; }
    AlarmState getAlarm() const { return _alarm; }

    float  getTemperature() const { return _temperature; }
    float  getSetpoint() const { return _setpoint; }
    float  getPower() const { return _power; }
    float  getElapsedSimS() const { return _elapsedSimS; }
    int8_t getSegment() const { return _lastSegment; }

    bool isProfileLoaded() const { return _profile.getPointCount() > 0; }

private:
    // Dependencias (nao proprietario)
    IFurnace*      _furnace;
    SSRDriver*     _ssr;
    PIDController* _pid;
    FaultDetector* _detector;
    Telemetry*     _telemetry;
    StatusLeds*    _leds;

    // Perfil
    KilnProfile _profile;

    // Estado
    KilnState  _state;
    FaultCode  _fault;
    AlarmState _alarm;

    // Grandezas
    float _temperature;
    float _setpoint;
    float _power;
    bool  _ssrOn;

    // Tempo
    float    _elapsedSimS;        // tempo simulado decorrido no perfil
    uint32_t _tsSimMs;            // tempo simulado desde o boot
    uint32_t _lastTelemetryTsMs;  // ts da ultima telemetria emitida

    // Controle de segmento
    int8_t _lastSegment;

    // Metodos internos
    void enterState(KilnState newState);
    void handleIdle(float dtSim);
    void handleStarting(float dtSim);
    void handleRunning(float dtSim);
    void handleFinished(float dtSim);
    void handleResetting(float dtSim);
    void handleEmergency(float dtSim);
    void evaluateFaults(float dtSim);
    void emitPeriodicTelemetry();
};

#endif // KILN_CONTROLLER_H