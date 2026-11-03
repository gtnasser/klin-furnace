#include "KilnController.h"
#include "Config.h"

// ------------------------------------------------------------
// Validacao do mapa de pinos
// ------------------------------------------------------------
static bool validatePins(const HardwarePins& p) {
    const uint8_t required[] = { p.ssr, p.ledRun, p.ledFault };

    for (uint8_t i = 0; i < 3; i++) {
        if (required[i] == PIN_UNASSIGNED) {
            return false;
        }
        if (required[i] > 39) {
            return false;
        }
    }

    if (p.ssr == p.ledRun || p.ssr == p.ledFault || p.ledRun == p.ledFault) {
        return false;
    }

    return true;
}

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
      _state(KilnState::EMERGENCY),
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
bool KilnController::begin(const HardwarePins& pins) {

    if (!validatePins(pins)) {
        _state  = KilnState::EMERGENCY;
        _fault  = FaultCode::NONE;
        _alarm  = AlarmState::NO_ALARM;
        _power  = 0.0f;
        _ssrOn  = false;
        return false;
    }

    _furnace->begin();
    _ssr->begin(pins.ssr);
    _pid->begin(PID_KP, PID_KI, PID_KD);
    _detector->reset();
    _leds->begin(pins.ledRun, pins.ledFault);

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
    return true;
}

// ------------------------------------------------------------
// Ciclo principal
// ------------------------------------------------------------
void KilnController::update(uint32_t dtRealMs) {

    const float dtSim = (dtRealMs / 1000.0f) * g_timeScale;

    _tsSimMs += (uint32_t)(dtSim * 1000.0f);

    _temperature = _furnace->getTemperature();

    switch (_state) {
        case KilnState::IDLE:      handleIdle(dtSim);      break;
        case KilnState::STARTING:  handleStarting(dtSim);  break;
        case KilnState::RUNNING:   handleRunning(dtSim);   break;
        case KilnState::FINISHED:  handleFinished(dtSim);  break;
        case KilnState::RESETTING: handleResetting(dtSim); break;
        case KilnState::EMERGENCY: handleEmergency(dtSim); break;
    }

    _ssr->update(_power, dtRealMs);
    _ssrOn = _ssr->isOn();

    _leds->update(_state, _fault, dtRealMs);

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
        return ProfileStatus::NOT_IDLE;
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
        return false;
    }

    _pid->reset();
    _detector->reset();
    _ssr->reset();
    _fault = FaultCode::NONE;
    _alarm = AlarmState::NO_ALARM;
    _power = 0.0f;
    _elapsedSimS = 0.0f;
    _lastSegment = -1;

    enterState(KilnState::RESETTING);
    return true;
}

// ------------------------------------------------------------
// Handlers de estado
// ------------------------------------------------------------
void KilnController::handleIdle(float dtSim) {
    _power = 0.0f;
    _setpoint = 0.0f;
    _furnace->setPower(0.0f, dtSim);
}

void KilnController::handleStarting(float dtSim) {
    _power = 0.0f;
    _furnace->setPower(0.0f, dtSim);

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

void KilnController::handleRunning(float dtSim) {

    _elapsedSimS += dtSim;
    const float elapsedMin = _elapsedSimS / 60.0f;

    _setpoint = _profile.getSetpoint(elapsedMin);

    const int8_t seg = _profile.getSegmentIndex(elapsedMin);
    if (seg != _lastSegment) {
        _pid->resetIntegral();
        _lastSegment = seg;
        _telemetry->emitEvent(_tsSimMs, "SEGMENT", _state, _fault);
    }

    _power = _pid->update(_setpoint, _temperature, dtSim);

    _furnace->setPower(_power, dtSim);

    evaluateFaults(dtSim);
    if (_state == KilnState::EMERGENCY) {
        return;
    }

    if (_profile.isFinished(elapsedMin)) {
        _power = 0.0f;
        _furnace->setPower(0.0f, dtSim);
        enterState(KilnState::FINISHED);
    }
}

void KilnController::handleFinished(float dtSim) {
    _power = 0.0f;
    _setpoint = _profile.getSetpoint(_profile.getDurationMin());
    _furnace->setPower(0.0f, dtSim);

    evaluateFaults(dtSim);
}

void KilnController::handleResetting(float dtSim) {
    _power = 0.0f;
    _setpoint = 0.0f;
    _furnace->setPower(0.0f, dtSim);

    if (_temperature <= RESET_ALLOWED_C) {
        _furnace->reset();
        _temperature = _furnace->getTemperature();
        enterState(KilnState::IDLE);
    }
}

void KilnController::handleEmergency(float dtSim) {
    _power = 0.0f;
    _ssr->forceOff();
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
// ------------------------------------------------------------
void KilnController::emitPeriodicTelemetry() {

    const uint32_t periodMs = (uint32_t)(g_telemetryPeriodS * 1000.0f);

    if (periodMs == 0) {
        return;
    }

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