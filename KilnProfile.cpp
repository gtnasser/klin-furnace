#include "KilnProfile.h"
#include "Config.h"

KilnProfile::KilnProfile() : _count(0) {
}

bool KilnProfile::load(const float (*points)[2], uint8_t count) {
    if (points == nullptr || count == 0 || count > MAX_PROFILE_POINTS) {
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

ProfileStatus KilnProfile::validate() const {

    if (_count < 2) {
        return ProfileStatus::TOO_FEW_POINTS;
    }

    if (_points[0][0] != 0.0f) {
        return ProfileStatus::FIRST_POINT_NOT_ZERO;
    }

    for (uint8_t i = 1; i < _count; i++) {
        const float tPrev    = _points[i - 1][0];
        const float tCurr    = _points[i][0];
        const float tempPrev = _points[i - 1][1];
        const float tempCurr = _points[i][1];

        if (tCurr <= tPrev) {
            return ProfileStatus::TIME_NOT_INCREASING;
        }

        if (tempCurr < tempPrev) {
            return ProfileStatus::TEMP_DECREASING;
        }

        if (tempCurr < MIN_VALID_TEMP_C || tempCurr > MAX_KILN_TEMP_C) {
            return ProfileStatus::TEMP_OUT_OF_RANGE;
        }
    }

    if (_points[0][1] < MIN_VALID_TEMP_C ||
        _points[0][1] > MAX_KILN_TEMP_C) {
        return ProfileStatus::TEMP_OUT_OF_RANGE;
    }

    if (getDurationMin() <= 0.0f) {
        return ProfileStatus::DURATION_TOO_SHORT;
    }

    return ProfileStatus::OK;
}

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

            const float frac = (elapsedMin - t0) / span;
            return v0 + frac * (v1 - v0);
        }
    }

    return _points[_count - 1][1];
}

int8_t KilnProfile::getSegmentIndex(float elapsedMin) const {

    if (_count < 2) {
        return -1;
    }

    if (elapsedMin <= _points[0][0]) {
        return 0;
    }

    for (uint8_t i = 1; i < _count; i++) {
        if (elapsedMin <= _points[i][0]) {
            return (int8_t)(i - 1);
        }
    }

    return (int8_t)(_count - 2);
}

float KilnProfile::getDurationMin() const {
    if (_count == 0) {
        return 0.0f;
    }
    return _points[_count - 1][0];
}

bool KilnProfile::isFinished(float elapsedMin) const {
    if (_count == 0) {
        return true;
    }
    return elapsedMin >= getDurationMin();
}