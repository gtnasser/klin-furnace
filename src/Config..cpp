#include "Config.h"

// ============================================================
// Config.cpp - Dados de configuracao com alocacao unica
// ============================================================

// ------------------------------------------------------------
// Perfil rapido: 5 min, aquecimento ate 100 degC
// ------------------------------------------------------------
const float FAST_BURN[][2] = {
    {  0.0f,  25.0f },
    {  2.0f,  60.0f },
    {  4.0f, 100.0f },
    {  5.0f, 100.0f }
};

static_assert(
    (sizeof(FAST_BURN) / sizeof(FAST_BURN[0])) <= 255,
    "FAST_BURN excede 255 pontos: count truncaria em uint8_t"
);

static_assert(
    (sizeof(FAST_BURN) / sizeof(FAST_BURN[0])) <= 128,
    "FAST_BURN excede 128 pontos: getSegmentIndex() (int8_t) transborda"
);

// ------------------------------------------------------------
// Perfil regular: 435 min, aquecimento ate 900 degC
// ------------------------------------------------------------
const float REGULAR_BURN[][2] = {
    {   0.0f,   25.0f },
    {  60.0f,  200.0f },
    { 180.0f,  600.0f },
    { 300.0f,  850.0f },
    { 420.0f,  900.0f },
    { 435.0f,  900.0f }
};

static_assert(
    (sizeof(REGULAR_BURN) / sizeof(REGULAR_BURN[0])) <= 255,
    "REGULAR_BURN excede 255 pontos: count truncaria em uint8_t"
);

static_assert(
    (sizeof(REGULAR_BURN) / sizeof(REGULAR_BURN[0])) <= 128,
    "REGULAR_BURN excede 128 pontos: getSegmentIndex() (int8_t) transborda"
);

// ------------------------------------------------------------
// Tabela de perfis
// Slots vazios: points == nullptr, count == 0.
// O nome do slot vazio e "VAZIO" para aparecer na listagem.
// ------------------------------------------------------------
const ProfileEntry PROFILE_TABLE[PROFILE_COUNT] = {
    { "FAST_PROFILE",  FAST_BURN,
      sizeof(FAST_BURN) / sizeof(FAST_BURN[0]) },
    { "REGULAR_BURN",  REGULAR_BURN,
      sizeof(REGULAR_BURN) / sizeof(REGULAR_BURN[0]) },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 },
    { "VAZIO",         nullptr, 0 }
};