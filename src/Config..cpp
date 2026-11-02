#include "Config.h"

// ============================================================
// Config.cpp - Dados de configuracao com alocacao unica
// ============================================================

// ------------------------------------------------------------
// Perfil de producao: 435 min, aquecimento ate 900 degC
// ------------------------------------------------------------
const float PRODUCTION_POINTS[][2] = {
    {   0.0f,   25.0f },
    {  60.0f,  200.0f },
    { 180.0f,  600.0f },
    { 300.0f,  850.0f },
    { 420.0f,  900.0f },
    { 435.0f,  900.0f }
};

static_assert(
    (sizeof(PRODUCTION_POINTS) / sizeof(PRODUCTION_POINTS[0])) <= 255,
    "PRODUCTION_POINTS excede 255 pontos: _LEN truncaria em uint8_t"
);

static_assert(
    (sizeof(PRODUCTION_POINTS) / sizeof(PRODUCTION_POINTS[0])) <= 128,
    "PRODUCTION_POINTS excede 128 pontos: getSegmentIndex() (int8_t) transborda"
);

const uint8_t PRODUCTION_POINTS_LEN =
    sizeof(PRODUCTION_POINTS) / sizeof(PRODUCTION_POINTS[0]);

// ------------------------------------------------------------
// Perfil de teste: 10 min, aquecimento ate 300 degC
// ------------------------------------------------------------
const float TEST_POINTS[][2] = {
    {  0.0f,  25.0f },
    {  2.0f, 100.0f },
    {  5.0f, 200.0f },
    {  8.0f, 280.0f },
    { 10.0f, 300.0f }
};

static_assert(
    (sizeof(TEST_POINTS) / sizeof(TEST_POINTS[0])) <= 255,
    "TEST_POINTS excede 255 pontos: _LEN truncaria em uint8_t"
);

static_assert(
    (sizeof(TEST_POINTS) / sizeof(TEST_POINTS[0])) <= 128,
    "TEST_POINTS excede 128 pontos: getSegmentIndex() (int8_t) transborda"
);

const uint8_t TEST_POINTS_LEN =
    sizeof(TEST_POINTS) / sizeof(TEST_POINTS[0]);