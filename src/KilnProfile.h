#ifndef KILN_PROFILE_H
#define KILN_PROFILE_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// KilnProfile - Perfil de queima por pontos (tempo, temperatura)
//
// Contrato:
//   - Pontos em (minutos, degC), ordenados por tempo crescente
//   - Primeiro ponto DEVE ter t = 0
//   - Tempos estritamente crescentes (sem duplicatas)
//   - Temperaturas nao decrescentes
//   - Maximo de MAX_PROFILE_POINTS pontos
//   - getSegmentIndex() retorna int8_t: limite de 127 segmentos
// ============================================================

#define MAX_PROFILE_POINTS  16

class KilnProfile {
public:
    KilnProfile();

    bool load(const float (*points)[2], uint8_t count);
    ProfileStatus validate() const;

    float  getSetpoint(float elapsedMin) const;
    int8_t getSegmentIndex(float elapsedMin) const;
    float  getDurationMin() const;
    bool   isFinished(float elapsedMin) const;

    uint8_t getPointCount() const { return _count; }
    bool    isEmpty() const { return _count == 0; }

private:
    float   _points[MAX_PROFILE_POINTS][2];
    uint8_t _count;
};

#endif // KILN_PROFILE_H