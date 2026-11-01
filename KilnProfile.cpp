#include "KilnProfile.h"
#include "Config.h"

// ------------------------------------------------------------
// Construtor
// ------------------------------------------------------------
KilnProfile::KilnProfile() : _count(0) {
    for (uint8_t i = 0; i < 16; i++) {
        _points[i][0] = 0.0f;
        _points[i][1] = 0.0f;
    }
}

// ------------------------------------------------------------
// Carrega os pontos
// ------------------------------------------------------------
bool KilnProfile::load(const float (*points)[2], uint8_t count) {
    if (points == nullptr || count < 2 || count > 16) {
        _count = 0;
        return false;
    }
    for (uint8_t i = 0; i < count; i++) {
        _points[i][0] = points[i][0];
        _points[i][1] = points[i][1];
    }
    _count = count;
    return true;
}

// ------------------------------------------------------------
// Validacao
// ------------------------------------------------------------
ProfileStatus KilnProfile::validate() const {
    if (_count < 2) {
        return ProfileStatus::TOO_FEW_POINTS;
    }
    if (_points[0][0] != 0.0f) {
        return ProfileStatus::TIME_ZERO_START;
    }
    for (uint8_t i = 1; i < _count; i++) {
        if (_points[i][0] <= _points[i - 1][0]) {
            return ProfileStatus::TIME_NOT_INCREASING;
        }
        if (_points[i][1] < _points[i - 1][1]) {
            return ProfileStatus::TEMP_DECREASING;
        }
    }
    for (uint8_t i = 0; i < _count; i++) {
        if (_points[i][1] < MIN_VALID_TEMP_C ||
            _points[i][1] > MAX_KILN_TEMP_C) {
            return ProfileStatus::TEMP_OUT_OF_RANGE;
        }
    }
    return ProfileStatus::OK;
}

// ------------------------------------------------------------
// Setpoint por interpolacao linear
// ------------------------------------------------------------
float KilnProfile::getSetpoint(float elapsedMin) const {
    if (_count == 0) {
        return 0.0f;
    }
    if (elapsedMin <= _points[0][0]) {
        return _points[0][1];
    }
    if (elapsedMin >= _points[_count - 1][0]) {
        return _points[_count - 1][1];
    }
    for (uint8_t i = 1; i < _count; i++) {
        if (elapsedMin <= _points[i][0]) {
            const float t0 = _points[i - 1][0];
            const float t1 = _points[i][0];
            const float v0 = _points[i - 1][1];
            const float v1 = _points[i][1];
            const float span = t1 - t0;
            if (span <= 0.0f) {
                return v1;
            }
            const float ratio = (elapsedMin - t0) / span;
            return v0 + ratio * (v1 - v0);
        }
    }
    return _points[_count - 1][1];
}

// ------------------------------------------------------------
// Duracao total
// ------------------------------------------------------------
float KilnProfile::getDurationMin() const {
    if (_count == 0) {
        return 0.0f;
    }
    return _points[_count - 1][0];
}

// ------------------------------------------------------------
// Indice do segmento atual
// ------------------------------------------------------------
int8_t KilnProfile::getSegmentIndex(float elapsedMin) const {
    if (_count < 2) {
        return -1;
    }
    if (elapsedMin < _points[0][0]) {
        return 0;
    }
    for (uint8_t i = 1; i < _count; i++) {
        if (elapsedMin < _points[i][0]) {
            return (int8_t)(i - 1);
        }
    }
    return (int8_t)(_count - 2);
}

// ------------------------------------------------------------
// Perfil concluido?
// ------------------------------------------------------------
bool KilnProfile::isFinished(float elapsedMin) const {
    if (_count == 0) {
        return false;
    }
    return elapsedMin >= _points[_count - 1][0];
}