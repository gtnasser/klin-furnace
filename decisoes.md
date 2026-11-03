# Decisões de projeto — Forno ESP32

- Registro das decisões tomadas durante o desenvolvimento do firmware.
- Última atualização: novembro de 2026.

## Plataforma e escopo

| # | Decisão |
|---|---|
| D01 | V1 roda no simulador Wokwi, com ESP32 e Arduino core |
| D02 | V1 é apenas aquecimento; resfriamento forçado fica para versão futura, mas a arquitetura prevê o encaixe |
| D03 | V2 roda na bancada com termopar real |
| D04 | Termopar adotado: MAX31855 (SPI), por ter detecção de falha embutida |
| D05 | Resolução de temperatura: 1 °C em todo o sistema |

## Arquitetura

| # | Decisão |
|---|---|
| D06 | Objetos globais estáticos no `main.cpp` + injeção por ponteiro no construtor do `KilnController` |
| D07 | `Config.h` mantém `#define` para o fixo e `extern` para as três globais mutáveis (`g_timeScale`, `g_telemetryPeriodS`, `g_activeProfile`) |
| D08 | Perfis como `const float [][2]` definidos em `Config.cpp`, declarados `extern` no `Config.h` |
| D09 | `ProfileId` como `enum : uint8_t` |
| D10 | `INVALID_PROFILE` saiu do `FaultCode` e virou `ProfileStatus`, retornado por `validate()` |
| D11 | `FaultDetector::update()` recebe `setpoint` e `dtSim` |
| D12 | `SENSOR_STUCK` exige três condições simultâneas: aquecendo, plano e longe do alvo |
| D13 | `SSR_STUCK_ON` e `HEATING_FAILURE` ficam declarados no V1 sem geração, com comentário explícito |
| D14 | `SimulatedFurnace` recebe potência média do PID, não o binário do SSR |
| D24 | `heating` no `FaultDetector` usa `power` apenas, ignorando `ssrOn` (SSR pulsado zeraria o timer) |
| D25 | `KilnController::begin()` recebe `const HardwarePins&`; o controlador não conhece hardware diretamente |
| D26 | `FaultCode::NONE` explícito no índice 0 do enum |
| D27 | Funções `*ToString` inline nos headers, para uso direto na telemetria |
| D32 | Arrays de perfil em `Config.cpp` com `extern` no header: uma única cópia no binário |
| D33 | `_LEN` calculado por `sizeof` e protegido por `static_assert` (limites de 255 e 128) |
| D34 | `HardwarePins` centraliza os pinos; assinatura de `begin()` estável quando o V2 crescer |
| D35 | `PIN_UNASSIGNED` (0xFF) como sentinela de pino não configurado |
| D36 | `KilnController::begin()` valida pinos e retorna `bool`; falha deixa o controlador em `EMERGENCY` |
| D37 | `_state` inicializa em `EMERGENCY`, não em `IDLE`: sem `begin()` válido, o forno nunca aquece |
| D38 | `SSRDriver` e `StatusLeds` ignoram escrita quando o pino é `PIN_UNASSIGNED` |
| D39 | `ProfileStatus::NOT_IDLE` distingue recusa por estado de recusa por perfil inválido |
| D40 | `ProfileStatus::FIRST_POINT_NOT_ZERO` em vez de reutilizar `TIME_NOT_INCREASING` |

## Configuração e operação

| # | Decisão |
|---|---|
| D15 | `TIME_SCALE` e período de telemetria são parâmetros mutáveis, não constantes |
| D16 | Perfil ativo: padrão por compilação, sobrescrita em runtime |
| D17 | Dois perfis: `PRODUCTION` (435 min) e `TEST` (10 min) |
| D18 | Telemetria em JSON por linha, emitida no fim do ciclo do controlador |
| D19 | Frequência de telemetria contada em tempo simulado |
| D20 | Interface do V1: serial no firmware + ponte serial→navegador fora do firmware |
| D28 | `PROFILE_DEFAULT = PROFILE_TEST` durante o desenvolvimento |
| D29 | Telemetria periódica compara `_tsSimMs` com `_lastTelemetryTsMs` (sem acumulador flutuante) |
| D30 | Comandos seriais: `S`, `R`, `T`, `I`, `X<n>`, `P<n>`, `?` |
| D41 | Limites dos comandos (`TIME_SCALE_MIN/MAX`, `TELEMETRY_PERIOD_MIN/MAX`) centralizados no `Config.h` |
| D42 | `Telemetry` emite `null` para `NaN`/`Inf`, mantendo o JSON válido |
| D43 | `TELEMETRY_ENABLED` permite desligar a telemetria por compilação sem remover as chamadas |

## Controle

| # | Decisão |
|---|---|
| D44 | Derivada do PID sobre a medição, não sobre o erro (evita derivative kick na troca de segmento) |
| D45 | Anti-windup condicional: bloqueia a integração só quando a saída está saturada na mesma direção do erro |
| D46 | `PIDController::update()` retorna a última saída quando `dtSim <= 0` |
| D47 | Integral zerado na troca de segmento (`resetIntegral()`), evitando arrasto entre rampas |
| D48 | `FaultDetector` prioriza `OVER_TEMPERATURE` sobre `SENSOR_STUCK` |
| D49 | `THERMOCOUPLE_OPEN` cobre leitura acima de `SAFETY_LIMIT_C + 200` (termopar em curto) |

## Hardware do V1

| # | Decisão |
|---|---|
| D21 | Sem botões, sem buzzer no V1 |
| D22 | LEDs de run e fault implementados; LED de ack reservado |
| D23 | Pinos do termopar definidos no `Config.h` mesmo sem uso |
| D31 | LED extra no pino do SSR para visualizar o time-proportioning no Wokwi |
| D50 | LED de ack e botões RESET/ACK presentes no `diagram.json` para o V2, sem ligação no firmware |
| D51 | LED de ack no GPIO 13, não no 15: o GPIO 15 é strap de boot e pisca durante a inicialização |
| D52 | Botões com pull-down externo de 10k, lógica ativa em HIGH; preferido ao `INPUT_PULLUP` por imunidade a ruído do SSR |

## Escala de tempo — assimetria intencional

| Domínio | Tempo | Motivo |
|---|---|---|
| Modelo térmico (`SimulatedFurnace`) | Simulado (`dt × TIME_SCALE`) | Permite acelerar a queima |
| PID e `FaultDetector` | Simulado | Consistente com o modelo térmico |
| `SSRDriver` | Real | O SSR físico opera em tempo real |
| `StatusLeds` | Real | Sinalização visual para humano |

Consequência: no Wokwi com escala 50, o forno esquenta 50x mais rápido, mas o LED do SSR pisca em ritmo de bancada. Esperado e correto.

## Pendências conhecidas para o V2

- `Kd = 15` é alto para resolução de 1 °C; provável necessidade de filtro na medição ou redução do ganho
- Modelo térmico de primeira ordem não tem inércia; ganhos do PID precisarão de reajuste no hardware real
- `SSR_STUCK_ON` requer realimentação física do SSR
- `HEATING_FAILURE` requer leitura real de temperatura
- Feedforward de rampa: somar potência necessária para seguir a rampa, deixando o PID corrigir só o desvio
- Botões RESET/ACK e LED de ack: hardware no diagrama com pull-down de 10k, lógica pendente (ativa em HIGH)
