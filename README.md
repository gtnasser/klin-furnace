# KLIN-FURNACE

Firmware em C++ para controle de forno elétrico por perfil de queima, com PID, detecção de falhas e telemetria serial.

**V1**: validado em simulação (Wokwi), apenas aquecimento.
**V2**: bancada com termopar real MAX31855 (pendente).
**V3**: leituras analógicas (tensão de rede / calibração) — reservado.

---

## 1. O projeto

O KLIN-FURNACE é o firmware que controla um forno elétrico resistivo. Ele executa uma curva de temperatura programada (o "perfil de queima"), mantendo a temperatura interna o mais próximo possível do alvo em cada instante, e desliga o aquecimento de forma segura quando algo sai errado.

O que ele controla fisicamente:

- Um **SSR** (solid state relay) que liga e desliga a resistência do forno. O firmware não controla a potência de forma analógica: ele modula o tempo ligado dentro de uma janela fixa de 10 segundos (time-proportioning). 60% de potência significa 6 s ligado e 4 s desligado, repetidamente.
- Um **termopar tipo K** lido por um MAX31855 via SPI, que entrega a temperatura já digitalizada e com compensação de junta fria.
- Três **LEDs de status** (run, falha, ack) e, no V2, um buzzer, um display OLED em I²C, um encoder rotativo e dois botões físicos.
- Uma **porta serial** em 115200 baud, por onde saem a telemetria em JSON e entram os comandos do operador.

O que ele **não** faz: não controla a potência de forma contínua (o SSR é binário), não mede a temperatura ambiente, não tem relógio de tempo real e não persiste dados entre reinicializações. Os perfis ficam em flash, compilados junto com o firmware.

O objetivo do projeto é ser um controlador de bancada confiável e auditável: cada decisão do controlador é observável na telemetria, e cada falha tem um código próprio.

---

## 2. Como o software funciona

### 2.1 O ciclo de controle

A cada passagem do loop principal, o firmware executa sempre a mesma sequência:

1. Lê a temperatura do forno pela interface `IFurnace`. No V1 isso é um modelo térmico simulado; no V2 será o MAX31855 real.
2. Consulta o perfil ativo para descobrir qual é o setpoint daquele instante. O perfil é uma lista de pontos (tempo, temperatura) e o setpoint é obtido por interpolação linear entre dois pontos consecutivos.
3. Passa temperatura e setpoint ao PID, que devolve uma potência de 0 a 100%.
4. Passa a potência ao driver do SSR, que decide se o relé deve estar ligado ou desligado naquele milissegundo, dentro da janela de modulação.
5. Roda o detector de falhas sobre a temperatura, o setpoint e a potência.
6. Atualiza os LEDs e, se for hora, emite a telemetria.

O fluxo completo é: **perfil → setpoint → PID → potência → SSR**.

### 2.2 As duas escalas de tempo

O firmware trabalha com dois relógios diferentes, de propósito.

O **tempo simulado** (`dt × TIME_SCALE`) alimenta o modelo térmico, o PID e o detector de falhas. Com `TIME_SCALE = 50`, um segundo real vale 50 segundos simulados, o que permite rodar uma queima de 7 horas em pouco mais de 8 minutos.

O **tempo real** alimenta o SSR e os LEDs. O relé não pode ser acelerado: ele é um dispositivo físico e a janela de 10 s é o que protege a resistência.

Essa assimetria é intencional e é o que permite testar o controle rápido sem queimar o hardware.

### 2.3 A máquina de estados

O `KilnController` mantém um estado explícito, e só aceita transições válidas:

- **IDLE** — ocioso, SSR desligado, nenhum perfil rodando. É o único estado em que se pode trocar de perfil.
- **STARTING** — transição curta de partida, com validação do perfil antes de aquecer.
- **RUNNING** — perfil em execução, PID atuando, SSR modulando.
- **FINISHED** — perfil concluído, aquecimento desligado, aguardando resfriamento.
- **RESETTING** — transição de retorno ao IDLE, com espera de segurança.
- **EMERGENCY** — falha ativa, SSR travado em desligado, exige reset manual.

O reset só é permitido abaixo de 200 °C (`RESET_ALLOWED_C`). Acima disso, o firmware recusa com `TOO_HOT`: reiniciar um forno quente é perigoso.

### 2.4 A detecção de falhas

Três falhas estão ativas no V1:

- **THERMOCOUPLE_OPEN** — leitura inválida (NaN, Inf, ou abaixo de -50 °C). O termopar está desconectado ou em curto.
- **OVER_TEMPERATURE** — temperatura acima de 1150 °C. Proteção independente do perfil.
- **SENSOR_STUCK** — a temperatura não varia mais de 0,5 °C durante 300 s enquanto o PID está acima de 20% de potência e longe do alvo. O sensor travou ou soltou do forno.

Duas falhas estão previstas para o V2, quando houver realimentação física: `SSR_STUCK_ON` (o relé não desliga) e `HEATING_FAILURE` (aquece sem resposta).

### 2.5 A telemetria

Toda a informação sai pela serial como uma linha JSON por evento, terminada em `\n`. Não há texto livre: qualquer programa consegue ler a saída sem parser especial.

Existem dois tipos de linha: a **telemetria periódica**, emitida a cada `TELEMETRY_PERIOD_DEFAULT_S` segundos simulados, com o estado completo do processo; e os **eventos**, emitidos quando algo acontece (boot, mudança de estado, troca de segmento, falha, comando aceito ou recusado).

Leituras inválidas são emitidas como `null`, para que o JSON continue válido mesmo com o termopar desconectado.

---

## 3. Comandos seriais

Conecte o monitor serial a **115200 baud** e envie um caractere por vez.

| Comando | Ação |
|---|---|
| `S` | Inicia o perfil ativo |
| `R` | Solicita reset após falha ou fim |
| `T` | **Lista** os 10 perfis com índice, nome, pontos e duração |
| `T<n>` | **Seleciona** o perfil `n` (0–9), só em IDLE e se o slot tiver pontos |
| `I` | Imprime o estado completo atual |
| `X<n>` | Ajusta `TIME_SCALE` (1 a 500) |
| `P<n>` | Ajusta o período de telemetria em segundos simulados (1 a 600) |
| `?` | Mostra a ajuda |

Exemplos: `T0` (seleciona FAST_PROFILE), `T` (lista perfis), `X20` (escala 20x), `P30` (telemetria a cada 30 s simulados), `I` (estado atual).

### 3.1 Ordem de validação do `T<n>`

A primeira falha interrompe a sequência.

| Ordem | Verificação | Erro retornado |
|---|---|---|
| 1 | Índice dentro de 0–9 | `INVALID_INDEX` |
| 2 | Estado da máquina é IDLE | `NOT_IDLE` |
| 3 | Slot tem pontos carregados | `PROFILE_EMPTY` |
| 4 | Perfil passa na validação de curva (verificada no `S`) | `PROFILE_INVALID` |
| 5 | Tudo válido | `PROFILE_CHANGED` |

---

## 4. Telemetria e eventos

### 4.1 Telemetria periódica

```json
{"ts":125000,"st":"RUNNING","t":412.5,"sp":450.0,"p":68.3,"ssr":true,"seg":1,"el":125,"flt":"NONE","alm":"NONE"}
```

| Campo | Significado |
|---|---|
| ts | Tempo simulado desde o boot (ms) |
| st | Estado da máquina |
| t | Temperatura atual (°C) |
| sp | Setpoint atual (°C) |
| p | Potência do PID (0–100%) |
| ssr | Estado binário do SSR |
| seg | Segmento do perfil (0-based, -1 se n/a) |
| el | Tempo decorrido no perfil (s simulados) |
| flt | Falha ativa |
| alm | Estado do alarme |

### 4.2 Eventos

```json
{"ts":0,"ev":"BOOT","st":"IDLE","flt":"NONE"}
{"ts":12000,"ev":"STATE","st":"RUNNING","flt":"NONE"}
{"ts":45000,"ev":"SEGMENT","st":"RUNNING","flt":"NONE"}
{"ts":98000,"ev":"FAULT","st":"RUNNING","flt":"SENSOR_STUCK"}
{"ev":"PROFILE_LIST","active":0,"profiles":[{"i":0,"name":"FAST_PROFILE","pts":4,"dur":5},{"i":1,"name":"REGULAR_BURN","pts":6,"dur":435}]}
{"ev":"PROFILE_CHANGED","index":1,"name":"REGULAR_BURN"}
{"ev":"PROFILE_DENIED","reason":"PROFILE_EMPTY","index":5}
```

Eventos possíveis:

```text
BOOT, READY, STATE, SEGMENT, FAULT, START, START_DENIED, RESET_OK, RESET_DENIED,
PROFILE_LIST, PROFILE_CHANGED, PROFILE_DENIED, PROFILE_INVALID,
TIMESCALE_SET, TIMESCALE_DENIED, TELEMPERIOD_SET, TELEMPERIOD_DENIED,
PIN_CONFIG_INVALID, INFO, HELP, UNKNOWN_CMD.
```

## 5. Roteiros de teste

Com `TIME_SCALE_DEFAULT = 1.0` e `TELEMETRY_PERIOD_DEFAULT_S = 3.0`, a simulação roda em tempo real e a telemetria sai a cada 3 s simulados — ritmo confortável para digitar comandos.

### 5.1 Teste 1 — Caminho feliz (FAST_PROFILE)
1. Compile e inicie a simulação.
2. Confirme no boot que o perfil ativo é o índice 0 (FAST_PROFILE).
3. Envie S.
4. Observe: LED verde acende, LED amarelo pisca, t sobe de 25 °C em direção a 100 °C.
5. Aos 5 min reais, o estado vai para FINISHED.
6. Envie R quando t cair abaixo de 200 °C.
7. Observe: o estado vai para RESETTING e depois IDLE.

### 5.2 Teste 2 — Listar e trocar perfis
1. Em IDLE, envie T. Deve imprimir PROFILE_LIST com os 10 slots.
2. Envie T1. Deve responder PROFILE_CHANGED com REGULAR_BURN.
3. Envie T5. Deve recusar com PROFILE_DENIED e reason: PROFILE_EMPTY.
4. Envie T15. Deve recusar com reason: INVALID_INDEX.
5. Envie S e depois T0. Deve recusar com reason: NOT_IDLE.
6. Envie R para voltar a IDLE e T0 para restaurar o FAST_PROFILE.

### 5.3 Teste 3 — Sobretemperatura
1. Envie T1 e depois S (perfil REGULAR_BURN).
2. Envie X50 para acelerar.
3. Aguarde t ultrapassar 1150 °C.
4. Observe: estado vai para EMERGENCY, LED vermelho pisca, SSR desliga.
5. Envie R antes de esfriar: deve recusar com TOO_HOT.

### 5.4 Teste 4 — Sensor travado
1. Em setup(), adicione furnace.injectTemperature(400.0f);.
2. Envie S.
3. O PID satura em 100% (temperatura fixa, longe do setpoint).
4. Após 300 s simulados, deve disparar SENSOR_STUCK.
5. Comente a linha de injeção para voltar ao normal.

### 5.5 Teste 5 — Perfil inválido
1. Em Config.cpp, altere um ponto do REGULAR_BURN para temperatura decrescente.
2. Envie T1 e depois S.
3. Observe: PROFILE_INVALID com reason: TEMP_DECREASING.
4. O firmware permanece em IDLE.
5. Reverta o valor.

### 5.6 Teste 6 — Comandos e limites
1. Envie X0: recusa com OUT_OF_RANGE.
2. Envie X20: aceita e reporta o novo valor.
3. Envie P0: recusa com OUT_OF_RANGE.
4. Envie P30: aceita.
5. Envie I: mostra o estado completo.
6. Envie ?: mostra a ajuda.

### 5.7 Teste 7 — Pino inválido (blindagem)
1. Em Config.h, altere LED_RUN_PIN para 4 (mesmo pino do LED_FAULT).
2. Compile e inicie.
3. Observe: PIN_CONFIG_INVALID no boot, estado EMERGENCY, nenhum LED acende.
4. Reverta o valor.

## 6. Perfis de queima

A tabela `PROFILE_TABLE[10]` fica em `Config.cpp`. Dois slots estão preenchidos; os outros oito estão vazios, reservados para carregamento futuro via NVS ou cartão SD.

Índice | Nome | Pontos | Duração | Temp. final
---|---|---|---|---
0 | FAST_PROFILE | 4 | 5 min | 100 °C
1 | REGULAR_BURN | 6 | 435 min | 900 °C
2–9 | VAZIO | 0 | — | —

**FAST_PROFILE** sobe 75 °C em 4 min (cerca de 17 °C/min) e mantém patamar por 1 min. É o perfil de teste rápido, com rampa suave o suficiente para o modelo térmico acompanhar.

**REGULAR_BURN** é o perfil de produção: rampa média de cerca de 2 °C/min até 900 °C, com patamar final de 15 min.

Slots vazios retornam `PROFILE_EMPTY` ao serem selecionados.

## 7. Arquitetura

O fluxo de dependências é unidirecional:
```text
main.cpp
   └── KilnController
         ├── KilnProfile
         ├── FaultDetector
         ├── PIDController
         ├── SSRDriver
         ├── IFurnace ──> SimulatedFurnace (V1)
         │                RealFurnace (V2)
         ├── Telemetry
         └── StatusLeds
```

`IFurnace` é o único ponto de troca entre simulação e hardware real. Para migrar ao V2, basta criar `RealFurnace : public IFurnace` e trocar a declaração do objeto no `main.cpp`.

### 7.1 Assimetria de tempo (intencional)

Domínio | Tempo | Motivo
---|---|---
Modelo térmico, PID, detecção de falhas | Simulado (dt × TIME_SCALE) | Permite acelerar a queima
SSR e LEDs | Real | Dispositivos físicos e sinalização visual

Com `TIME_SCALE = 50`, o forno esquenta 50x mais rápido, mas o LED do SSR pisca em ritmo de bancada.

## 8. Configuração

Tudo em `src/Config.h`. Os dados que ocupam memória (arrays de perfil e a tabela) ficam em `src/Config.cpp` e são referenciados por `extern`.

### 8.1 Limites térmicos

Constante | Valor | Significado
---|---|---
MAX_KILN_TEMP_C | 1100 | Limite máximo do perfil
SAFETY_LIMIT_C | 1150 | Dispara OVER_TEMPERATURE
MIN_VALID_TEMP_C | -50 | Abaixo disso: leitura inválida
RESET_ALLOWED_C | 200 | Reset só abaixo desta temperatura

### 8.2 Controle

Constante | Valor | Significado
---|---|---
SSR_WINDOW_MS | 10000 | Janela de modulação do SSR
SSR_MIN_ON_MS | 100 | Pulso mínimo confiável
PID_KP / KI / KD | 2.5 / 0.02 / 15 | Ganhos do PID
STUCK_WINDOW_S | 300 | Janela de confirmação de sensor travado
TIME_SCALE_DEFAULT | 1.0 | Aceleração do tempo simulado
TELEMETRY_PERIOD_DEFAULT_S | 3.0 | Período de telemetria (s simulados)
PROFILE_COUNT | 10 | Slots na tabela de perfis
PROFILE_DEFAULT | 0 | Índice do perfil ativo na inicialização (FAST_PROFILE)

### 8.3 Limites dos comandos seriais

Constante | Valor | Significado
---|---|---
TIME_SCALE_MIN | 1 | Menor escala aceita por X<n>
TIME_SCALE_MAX | 500 | Maior escala aceita por X<n>
TELEMETRY_PERIOD_MIN | 1 | Menor período aceito por P<n> (s)
TELEMETRY_PERIOD_MAX | 600 | Maior período aceito por P<n> (s)

### 8.4 Documentação adicional
- `docs/DECISOES.md` — registro de todas as decisões de projeto
- `src/Config.h` — comentários sobre cada constante
- `src/Types.h` — enums, HardwarePins e conversões para texto

## 9. Alocação de pinos — ESP32 DevKit v4 / WROOM-32

Mapa completo de GPIOs, com a origem de cada pino na V1, direção elétrica e restrições de hardware. O projeto **não usa Wi-Fi nem ADC2**, então as restrições de ADC2 são inofensivas. As únicas restrições realmente críticas são os **strapping pins** e os **pinos somente-entrada** (34, 35, 36, 39).

### 9.1 Tabela de alocação de pinos

19 pinos alocados, na ordem de definição do `Config.h`

GPIO | Constante | Função | Direção | Versão | Observação
---|---|---|---|---|---
23 | LED_RUN_PIN | LED de run | Saída | V1 | Movido do GPIO 2 (strap de boot)
4 | LED_FAULT_PIN | LED de falha | Saída | V1 | ADC2 e touch, não usados
25 | SSR_PIN | Saída do SSR | Saída | V1 | ADC2, não usado
13 | LED_ACK_PIN | LED de ack de alarme | Saída | V2 | Movido do GPIO 15 (strap de boot)
16 | BUZZER_PIN | Buzzer ativo | Saída | V2 | Pino livre
32 | RESET_BUTTON_PIN | Botão de reset físico | Entrada | V2 | Pull-down externo 10k, ativo em HIGH
33 | ACK_BUTTON_PIN | Botão de reconhecimento | Entrada | V2 | Pull-down externo 10k, ativo em HIGH
18 | THERMO_CLK_PIN | MAX31855 CLK (SPI) | Saída | V2 | VSPI CLK nativo
17 | THERMO_CS_PIN | MAX31855 CS (SPI) | Saída | V2 | Movido do GPIO 5 (strap de boot)
19 | THERMO_DO_PIN | MAX31855 DO / MISO | Entrada | V2 | VSPI MISO nativo
21 | I2C_SDA_PIN | SDA do I²C | Bidirecional | V2 | Pino I²C padrão
22 | I2C_SCL_PIN | SCL do I²C | Saída | V2 | Pino I²C padrão
27 | ENCODER_A_PIN | Encoder canal A | Entrada | V2 | Suporta interrupção
14 | ENCODER_B_PIN | Encoder canal B | Entrada | V2 | Suporta interrupção
26 | ENCODER_BTN_PIN | Botão do encoder | Entrada | V2 | ADC2: digital ok
34 | ANALOG_MAINS_V_PIN | Tensão da rede (ADC1 ch6) | Entrada apenas | V3 | Sem pull interno
35 | ANALOG_CALIB_REF_PIN | Referência de calibração (ADC1 ch7) | Entrada apenas | V3 | Sem pull interno
36 | ANALOG_SPARE_1_PIN | Reserva analógica (ADC1 ch0) | Entrada apenas | V3 | Sem pull interno
39 | ANALOG_SPARE_2_PIN | Reserva analógica (ADC1 ch3) | Entrada apenas | V3 | Sem pull interno

### 9.2 Pinos proibidos — nunca usar

GPIO | Motivo | Consequência
---|---|---
6 a 11 | Ligados à memória flash SPI interna | Usar qualquer um trava ou corrompe o boot
0, 2, 5, 12, 15 | Strapping pins do modo de boot | Nível errado no reset impede o boot. GPIO 12 em HIGH pode impedir o boot completamente
1 e 3 | UART0, usados pelo monitor serial | Conflito com a telemetria em 115200 baud
20, 24, 28, 29, 30, 31 | Não expostos no DevKit v4 | Inacessíveis fisicamente

### 9.3 Resumo da ocupação

- 19 de 34 GPIOs utilizáveis alocados
- 0 pinos totalmente livres — o GPIO 26 é o único restante e está em uso como botão do encoder
- Para expansão futura, use um expansor I²C no barramento 21/22 ou libere um dos canais analógicos de reserva (36 ou 39)

### 9.4 Contexto de validação
- **V1 — validada no Wokwi**: apenas aquecimento (SSR), LEDs de run e falha.
- **V2 — hardware real**: termopar MAX31855 via SPI, display OLED via I²C, encoder rotativo e botões físicos.
- **ADC2 sem impacto**: o projeto não usa ADC nem Wi-Fi, então as restrições de ADC2 nos pinos 4, 25, 13, 26 e 14 são inofensivas.
- **Pinos 34, 35, 36 e 39**: somente entrada e sem pull interno — use pull externo se precisar de nível definido.

### 9.5 Pontos de atenção no `diagram.json`

- **Os botões usam pull-down externo de 10k**. Cada botão liga o GPIO ao resistor, e o resistor ao GND. Solto: pino em LOW. Pressionado: pino em HIGH. A lógica do V2 é **ativa em HIGH** (`pinMode(pin, INPUT)` sem `PULLUP`).
- **O LED de ack está no GPIO 13, não no 15**. O GPIO 15 é strapping pin e pisca durante o boot do ESP32.
- **O LED de run está no GPIO 23, não no 2**. O GPIO 2 também é strapping pin, e um LED com resistor de 220 Ω pode impedir o boot porque o pull-up interno (cerca de 45 kΩ) não vence a corrente do LED.
- **Os dois botões compartilham `esp:GND.2`**. O DevKit tem dois pinos GND acessíveis na fileira direita; se o Wokwi reclamar de conexão duplicada, use `esp:GND.3`.

## 10. Build e simulação

### 10.1 Com PlatformIO (recomendado)

```bash
pio run                  # Compilar
pio run --target upload  # Compilar e enviar
pio device monitor       # Monitor serial
```

### 10.2 Com Arduino IDE
- Instale o suporte a ESP32 no Boards Manager.
- Selecione a placa ESP32 Dev Module.
- Copie todos os arquivos de src/ para uma pasta de sketch com o mesmo nome.
- Compile e envie.

### 10.3 Simulação no Wokwi

VS Code: 
- Instale a extensão Wokwi for VS Code.
- Ative a licença gratuita com Wokwi: Request a License, abra o projeto e pressione F1 → Wokwi: Start Simulator.

Wokwi Web
- Acesse `wokwi.com` e crie um projeto ESP32.
- Cole o conteúdo de **diagram.json** na aba **diagram.json**.
- Cole os arquivos de src/ no editor de código.
- Renomeie **main.cpp** para **sketch.ino**.
- Clique em **Play**.

⚠️ O Wokwi cria nomes com sufixo (por exemplo Config..cpp) ao colar um arquivo com nome já existente. Confira a lista de arquivos antes de compilar.

### 10.4 O que observar no diagrama

Componente | Pino | Significado
---|---|---
LED verde (RUN) | 23 | Aceso em RUNNING/FINISHED, pisca em STARTING/RESETTING
LED vermelho (FAULT) | 4 | Pisca quando há falha ativa
LED amarelo (SSR) | 25 | Mostra a modulação por time-proportioning
LED azul (ACK) | 13 | Reservado V2: reconhecimento de alarme
Botão RESET | 32 | Reservado V2: reset físico (pull-down 10k)
Botão ACK | 33 | Reservado V2: reconhecimento de alarme (pull-down 10k)
O Wokwi não simula forno nem termopar. A temperatura só aparece na telemetria serial.

## 11. Estado atual e pendências

### 11.1 Estado atual

Item | Situação
---|---
Controle PID com anti-windup | Implementado
Perfil de queima por pontos | Implementado
Tabela de 10 perfis em flash | Implementado
Detecção de falhas (3 ativas) | Implementado
Telemetria JSON por linha | Implementado
Modulação SSR (time-proportioning) | Implementado
Comandos seriais | Implementado
Simulação térmica | Implementado
Leitura real de termopar | Pendente V2
Botões, buzzer, display | Pendente V2
Resfriamento forçado | Pendente V2
Leituras analógicas (rede/calibração) | Pendente V3

### 11.2 Pendências para o V2

- RealFurnace com MAX31855 (SPI)
- Reajuste dos ganhos do PID para hardware real (inércia térmica)
- Filtro na medição ou redução de Kd (resolução de 1 °C gera ruído)
- `SSR_STUCK_ON` com realimentação física
- `HEATING_FAILURE` com leitura real de temperatura
- Botões de reset e ack, buzzer, display
- Feedforward de rampa
- Resfriamento forçado

### 11.3 Pendências para o V3

- Carregamento de perfis via NVS ou cartão SD
- Comando U<n> para upload de perfil por serial
- Leitura de tensão da rede (`ANALOG_MAINS_V_PIN`)
- Calibração do ADC com referência TL431 (`ANALOG_CALIB_REF_PIN`)

## 12. Estrutura do projeto

```text
forno-esp32/
├── platformio.ini                      # ou sketch.yaml para Arduino IDE
├── diagram.json                        # Wokwi
├── docs/
│   └── DECISOES.md
├── README.md
└── src/
    ├── Config.h/.cpp                   # Constantes globais + tabela de perfis
    ├── Types.h                         # Enums de estado, alarme, falha + ProcessSnapshot
    ├── KilnProfile.h/.cpp              # Perfil de queima + interpolação
    ├── FaultDetector.h/.cpp            # Detecção de falhas
    ├── PIDController.h/.cpp            # PID com anti-windup
    ├── SSRDriver.h/.cpp                # Time-proportioning do SSR
    ├── IFurnace.h                      # Interface abstrata do forno
    ├── SimulatedFurnace.h/.cpp         # Modelo térmico simulado
    ├── Telemetry.h/.cpp                # Emissão de telemetria JSON
    ├── StatusLeds.h/.cpp               # LEDs de status
    ├── KilnController.h/.cpp           # Máquina de estados + orquestração
    └── main.cpp                        # Setup/loop Arduino (renomear para sketch.ino no Wokwi)
```

