#ifndef KILN_PROFILE_H
#define KILN_PROFILE_H

#include <Arduino.h>
#include "Types.h"

// ============================================================
// KilnProfile - Perfil de queima por pontos (tempo, temperatura)
// V1: aquecimento apenas (temperatura monotonicamente crescente)
// ============================================================

class KilnProfile {
public:
    // Construtor vazio: perfil invalido ate load() ser chamado
    KilnProfile();

    // Carrega os pontos. Copia os dados para uso interno.
    // Retorna false se o ponteiro for nulo ou o tamanho invalido.
    bool load(const float (*points)[2], uint8_t count);

    // Valida o perfil carregado.
    ProfileStatus validate() const;

    // Setpoint para o tempo decorrido (em minutos).
    // Antes de t=0: retorna o primeiro ponto.
    // Depois do ultimo ponto: retorna o ultimo ponto (patamar).
    float getSetpoint(float elapsedMin) const;

    // Duracao total do perfil em minutos.
    float getDurationMin() const;

    // Indice do segmento atual (0-based). -1 se invalido.
    int8_t getSegmentIndex(float elapsedMin) const;

    // Numero de pontos carregados.
    uint8_t getPointCount() const { return _count; }

    // Indica se o tempo decorrido ultrapassou a duracao total.
    bool isFinished(float elapsedMin) const;

private:
    float   _points[16][2];  // copia interna, maximo 16 pontos
    uint8_t _count;
};

#endif // KILN_PROFILE_H