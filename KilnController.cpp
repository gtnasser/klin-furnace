#include "KilnController.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
KilnController::KilnController(IFurnace* furnace,
                               SSRDriver* ssr,
                               PIDController* pid,
                               FaultDetector* detector,
                               Telemetry* telemetry,
                               StatusLeds* leds)
    : _furnace(furnace),
      _ssr(ssr),
      _pid(pid),
      _detector(detector),
      _telemetry(telemetry),
      _leds(leds),
      _state(KilnState::IDLE),
      _fault(FaultCode::NONE),
      _alarm(AlarmState::NO_ALARM),
      _temperature(0.0f),
      _setpoint(0.0f),
      _power(0.0f),
      _ssrOn(false),
      _elapsedSimS(0.0f),
      _tsSimMs(0),
      _lastTelemetryTsMs(0),
      _lastSegment(-1) {
}

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
void KilnController::begin() {
    _furnace->begin();
    _ssr->begin(SSR_PIN);
    _pid->begin(PID_KP, PID_KI, PID_KD);
    _detector->reset();
    _leds->begin(LED_RUN_PIN, LED_FAULT_PIN);

    _state             = KilnState::IDLE;
    _fault             = FaultCode::NONE;
    _alarm             = AlarmState::NO_ALARM;
    _temperature       = _furnace->getTemperature();
    _setpoint          = 0.0f;
    _power             = 0.0f;
    _ssrOn             = false;
    _elapsedSimS       = 0.0f;
    _tsSimMs           = 0;
    _lastTelemetryTsMs = 0; 
    _lastSegment       = -1;

    _telemetry->emitEvent(_tsSimMs, "BOOT", _state, _fault);
}

// ------------------------------------------------------------
// Ciclo principal
// ------------------------------------------------------------
void KilnController::update(uint32_t dtRealMs) {

    // Converte para tempo simulado
    const float dtSim = (dtRealMs / 1000.0f) * g_timeScale;

    // Avanca o relogio simulado
    _tsSimMs += (uint32_t)(dtSim * 1000.0f);

    // Le a temperatura atual
    _temperature = _furnace->getTemperature();

    // Despacha para o handler do estado
    switch (_state) {
        case KilnState::IDLE:      handleIdle(dtSim);      break;
        case KilnState::STARTING:  handleStarting(dtSim);  break;
        case KilnState::RUNNING:   handleRunning(dtSim);   break;
        case KilnState::FINISHED:  handleFinished(dtSim);  break;
        case KilnState::RESETTING: handleResetting(dtSim); break;
        case KilnState::EMERGENCY: handleEmergency(dtSim); break;
    }

    // Atualiza atuador (tempo REAL: SSR opera em tempo real)
    _ssr->update(_power, dtRealMs);
    _ssrOn = _ssr->isOn();

    // Atualiza sinalizacao (tempo REAL)
    _leds->update(_state, _fault, dtRealMs);

    // Telemetria periodica
    emitPeriodicTelemetry();
}

// ------------------------------------------------------------
// Transicao de estado
// ------------------------------------------------------------
void KilnController::enterState(KilnState newState) {
    if (_state == newState) {
        return;
    }
    _state = newState;
    _telemetry->emitEvent(_tsSimMs, "STATE", _state, _fault);
}

// ------------------------------------------------------------
// Inicio do perfil
// ------------------------------------------------------------
ProfileStatus KilnController::start(const float (*points)[2], uint8_t count) {

    if (_state != KilnState::IDLE) {
        return ProfileStatus::OK;  // ja rodando; ignora silenciosamente
    }

    if (!_profile.load(points, count)) {
        return ProfileStatus::TOO_FEW_POINTS;
    }

    const ProfileStatus st = _profile.validate();
    if (st != ProfileStatus::OK) {
        _telemetry->emitEvent(_tsSimMs, "PROFILE_REJECTED", _state, _fault);
        return st;
    }

    _elapsedSimS = 0.0f;
    _lastSegment = -1;
    _pid->reset();
    _detector->reset();
    _fault = FaultCode::NONE;
    _alarm = AlarmState::NO_ALARM;

    enterState(KilnState::STARTING);
    return ProfileStatus::OK;
}

// ------------------------------------------------------------
// Reset apos falha
// ------------------------------------------------------------
bool KilnController::requestReset() {
    if (_state != KilnState::EMERGENCY && _state != KilnState::FINISHED) {
        return false;
    }
    if (_temperature > RESET_ALLOWED_C) {
        return false;  // forno ainda quente
    }

    _pid->reset();
    _detector->reset();
    _ssr->reset();          // libera o travamento do SSR
    _fault = FaultCode::NONE;
    _alarm = AlarmState::NO_ALARM;
    _power = 0.0f;
    _elapsedSimS = 0.0f;
    _lastSegment = -1;

    enterState(KilnState::RESETTING);
    return true;
}


// ------------------------------------------------------------
// IDLE: aguardando inicio
// ------------------------------------------------------------
void KilnController::handleIdle(float dtSim) {
    _power = 0.0f;
    _setpoint = 0.0f;
    _furnace->setPower(0.0f, dtSim);
}

// ------------------------------------------------------------
// STARTING: valida condicoes iniciais e parte para RUNNING
// ------------------------------------------------------------
void KilnController::handleStarting(float dtSim) {
    _power = 0.0f;
    _furnace->setPower(0.0f, dtSim);

    // Verifica se a leitura inicial e valida
    if (isnan(_temperature) || _temperature < MIN_VALID_TEMP_C) {
        _fault = FaultCode::THERMOCOUPLE_OPEN;
        _alarm = AlarmState::ACTIVE_UNACK;
        _ssr->forceOff();
        enterState(KilnState::EMERGENCY);
        return;
    }

    _setpoint = _profile.getSetpoint(0.0f);
    _elapsedSimS = 0.0f;
    _lastSegment = _profile.getSegmentIndex(0.0f);

    enterState(KilnState::RUNNING);
}

// ------------------------------------------------------------
// RUNNING: executa o perfil
// ------------------------------------------------------------
void KilnController::handleRunning(float dtSim) {

    // Avanca o tempo do perfil
    _elapsedSimS += dtSim;
    const float elapsedMin = _elapsedSimS / 60.0f;

    // Setpoint atual
    _setpoint = _profile.getSetpoint(elapsedMin);

    // Mudanca de segmento: zera o integral para evitar arrasto
    const int8_t seg = _profile.getSegmentIndex(elapsedMin);
    if (seg != _lastSegment) {
        _pid->resetIntegral();
        _lastSegment = seg;
        _telemetry->emitEvent(_tsSimMs, "SEGMENT", _state, _fault);
    }

    // PID
    _power = _pid->update(_setpoint, _temperature, dtSim);

    // Aplica potencia ao forno (tempo SIMULADO)
    _furnace->setPower(_power, dtSim);

    // Avalia falhas
    evaluateFaults(dtSim);
    if (_state == KilnState::EMERGENCY) {
        return;
    }

    // Fim do perfil?
    if (_profile.isFinished(elapsedMin)) {
        _power = 0.0f;
        _furnace->setPower(0.0f, dtSim);
        enterState(KilnState::FINISHED);
    }
}

// ------------------------------------------------------------
// FINISHED: perfil concluido, forno resfriando naturalmente
// ------------------------------------------------------------
void KilnController::handleFinished(float dtSim) {
    _power = 0.0f;
    _setpoint = _profile.getSetpoint(_profile.getDurationMin());
    _furnace->setPower(0.0f, dtSim);

    // Continua avaliando sobretemperatura e leitura invalida
    evaluateFaults(dtSim);
}

// ------------------------------------------------------------
// RESETTING: retorno ao estado seguro
// ------------------------------------------------------------
void KilnController::handleResetting(float dtSim) {
    _power = 0.0f;
    _setpoint = 0.0f;
    _furnace->setPower(0.0f, dtSim);

    // Quando o forno esfriar o suficiente, volta para IDLE
    if (_temperature <= RESET_ALLOWED_C) {
        _furnace->reset();
        _temperature = _furnace->getTemperature();
        enterState(KilnState::IDLE);
    }
}

// ------------------------------------------------------------
// EMERGENCY: SSR desligado, aguardando reset
// ------------------------------------------------------------
void KilnController::handleEmergency(float dtSim) {
    _power = 0.0f;
    _ssr->forceOff();

    // Mantem o modelo termico evoluindo (resfriamento natural)
    _furnace->setPower(0.0f, dtSim);
}

// ------------------------------------------------------------
// Avaliacao de falhas
// ------------------------------------------------------------
void KilnController::evaluateFaults(float dtSim) {
    const FaultCode f = _detector->update(
        _temperature, _setpoint, _power, _ssrOn, dtSim);

    if (f != FaultCode::NONE) {
        _fault = f;
        _alarm = AlarmState::ACTIVE_UNACK;
        _ssr->forceOff();
        _power = 0.0f;
        _telemetry->emitEvent(_tsSimMs, "FAULT", _state, _fault);
        enterState(KilnState::EMERGENCY);
    }
}

// ------------------------------------------------------------
// Telemetria periodica
// Compara o relogio simulado com o ultimo envio: imune a
// acumulacao de erro de ponto flutuante.
// ------------------------------------------------------------
void KilnController::emitPeriodicTelemetry() {

    const uint32_t periodMs = (uint32_t)(g_telemetryPeriodS * 1000.0f);

    // Periodo invalido: nao emite
    if (periodMs == 0) {
        return;
    }

    // Ainda nao passou o periodo
    if ((_tsSimMs - _lastTelemetryTsMs) < periodMs) {
        return;
    }

    _lastTelemetryTsMs = _tsSimMs;

    const uint32_t elapsedS = (uint32_t)(_elapsedSimS);
    const int8_t   seg      = (_state == KilnState::RUNNING ||
                               _state == KilnState::FINISHED)
                              ? _profile.getSegmentIndex(_elapsedSimS / 60.0f)
                              : -1;

    _telemetry->emit(_tsSimMs, _state,
                     _temperature, _setpoint, _power,
                     _ssrOn, seg, elapsedS,
                     _fault, _alarm);
}
