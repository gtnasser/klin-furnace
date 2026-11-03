# KLIN-FURNACE 

Firmware em C++ para controle de forno elétrico por perfil de queima, com PID, detecção de falhas e telemetria serial.

**V1**: validado em simulação (Wokwi), apenas aquecimento.
**V2**: bancada com termopar real MAX31855 (pendente).

---

## Estado atual

| Item | Situação |
|---|---|
| Controle PID com anti-windup | Implementado
| Perfil de queima por pontos | Implementado
| Detecção de falhas (3 ativas) | Implementado
| Telemetria JSON por linha | Implementado
| Modulação SSR (time-proportioning) | Implementado
| Comandos seriais | Implementado
| Simulação térmica | Implementado
| Leitura real de termopar | ⬜ V2
| Botões, buzzer, display | ⬜ V2
| Resfriamento forçado | ⬜ V2

---

## Estrutura do projeto

forno-esp32/
├── platformio.ini          (ou sketch.yaml para Arduino IDE)
├── diagram.json            (Wokwi)
├── docs/
│   └── DECISOES.md
├── README.md
└── src/
    ├── Config.h/.cpp                   # Constantes globais (GPIOs, limites, PID, SSR)
    ├── Types.h                         # Enums de estado, alarme, falha + ProcessSnapshot
    ├── KilnProfile.h/.cpp              # Perfil de queima + interpolação
    ├── FaultDetector.h/.cpp            # Detecção de falhas
    ├── PIDController.h.cpp             # PID com anti-windup
    ├── SSRDriver.h/.cpp                # Time-proportioning do SSR
    ├── IFurnace.h                      # Interface abstrata do forno
    ├── SimulatedFurnace.h/.cpp         # Modelo térmico simulado
    ├── Telemetry.h/.cpp                #
    ├── StatusLeds.h/.cpp               # LED de status
    ├── KilnController.h/.cpp           # Máquina de estados + orquestração
    └── main.cpp                        #  Setup/loop Arduino


---

## Build

### Com PlatformIO (recomendado)

```bash
# Compilar
pio run

# Compilar e enviar para a placa
pio run --target upload

# Monitor serial
pio device monitor
```

### Com Arduino IDE

- Instale o suporte a ESP32 no Boards Manager.
- Selecione a placa ESP32 Dev Module.
- Copie todos os arquivos de src/ para uma pasta de sketch com o mesmo nome.
- Compile e envie.

### Simulação no Wokwi

VS Code
- Instale a extensão **Wokwi for VS Code**.
- Ative a licença gratuita com o comando **Wokwi**: Request a License.
- Abra o projeto e pressione **F1 → Wokwi**: Start Simulator.

Wokwi Web
- Acesse `wokwi.com` e crie um projeto ESP32.
- Cole o conteúdo de **diagram.json** na aba **diagram.json**.
- Cole os arquivos de src/ no editor de código.
- Clique em **Play**.

O que observar no diagrama

Componente | Pino | Significado
--- | --- | ---
LED verde (RUN) | 2 | Aceso em RUNNING/FINISHED, pisca em STARTING/RESETTING
LED vermelho (FAULT) | 4 | Pisca quando há falha ativa
LED amarelo (SSR) | 25 | Mostra a modulação por time-proportioning
LED azul (ACK) | 13 | Reservado V2: reconhecimento de alarme
Botão RESET | 32 | Reservado V2: reset físico
Botão ACK | 33 | Reservado V2: reconhecimento de alarme

O Wokwi não simula forno nem termopar. A temperatura só aparece na telemetria serial.

---

## Comandos seriais

Conecte o monitor serial a **115200 baud** e envie um caractere por vez.

Comando | Ação
--- | ---
S | Inicia o perfil ativo
R | Solicita reset após falha ou fim
T | Alterna entre PRODUCTION e TEST (só em IDLE)
I | Imprime o estado completo atual
X<n> | Ajusta **TIME_SCALE** (1 a 500)
P<n> | Ajusta período de telemetria em segundos simulados (1 a 600)
? | Mostra a ajuda

Exemplos: X20 (escala 20x), P30 (telemetria a cada 30 s simulados), I (estado atual).

---

## Telemetria

Uma linha JSON por evento, terminada em `\n`.

Telemetria periódica

```json
{"ts":125000,"st":"RUNNING","t":412.5,"sp":450.0,"p":68.3,"ssr":true,"seg":1,"el":125,"flt":"NONE","alm":"NONE"}
```

Código json
--- | ---
Campo | Significado
ts | Tempo simulado desde o boot (ms)
st | Estado da máquina
t | Temperatura atual (°C)
sp | Setpoint atual (°C)
p | Potência do PID (0–100%)
ssr | Estado binário do SSR
seg | Segmento do perfil (0-based, -1 se n/a)
el | Tempo decorrido no perfil (s simulados)
flt | Falha ativa
alm | Estado do alarme

Leituras inválidas (**NaN/Inf**) são emitidas como null, mantendo o JSON válido.


---

## Eventos

```json
{"ts":0,"ev":"BOOT","st":"IDLE","flt":"NONE"}
{"ts":12000,"ev":"STATE","st":"RUNNING","flt":"NONE"}
{"ts":45000,"ev":"SEGMENT","st":"RUNNING","flt":"NONE"}
{"ts":98000,"ev":"FAULT","st":"RUNNING","flt":"SENSOR_STUCK"}
```

Eventos possíveis: 

```
BOOT, READY, STATE, SEGMENT, FAULT, START, START_DENIED, RESET_OK, RESET_DENIED, PROFILE_CHANGED, PROFILE_CHANGED_DENIED, PROFILE_INVALID, PROFILE_REJECTED, TIMESCALE_SET, TIMESCALE_DENIED, TELEMPERIOD_SET, TELEMPERIOD_DENIED, PIN_CONFIG_INVALID, INFO, HELP, UNKNOWN_CMD.
```


---

## Roteiro de teste

Teste 1 — Perfil de teste (caminho feliz)

1. Compile e inicie a simulação.
2. Envie S.
3. Observe: LED verde acende, LED amarelo pisca, telemetria mostra t subindo.
4. Após ~12 s reais (10 min simulados a 50x), estado vai para FINISHED.
5. Envie R quando a temperatura cair abaixo de 200 °C.
6. Observe: estado vai para RESETTING e depois IDLE.

Teste 2 — Sobretemperatura

1. Envie X1 (tempo real) para observar com calma.
2. Envie S.
3. Aguarde a temperatura ultrapassar 1150 °C.
4. Observe: estado vai para EMERGENCY, LED vermelho pisca, SSR desliga.
5. Envie R antes de esfriar: deve recusar com TOO_HOT.

Teste 3 — Sensor travado

1. No código, adicione em setup(): furnace.injectTemperature(400.0f);
2. Envie S.
3. O PID vai saturar em 100% (temperatura fixa, longe do setpoint).
4. Após 300 s simulados, deve disparar SENSOR_STUCK.
5. Comente a linha de injeção para voltar ao normal.

Teste 4 — Perfil inválido

1. No Config.cpp, altere um ponto do TEST_POINTS para temperatura decrescente.
2. Envie S.
3. Observe: PROFILE_INVALID com reason TEMP_DECREASING.
4. O firmware permanece em IDLE.

Teste 5 — Comandos

1. Envie T em IDLE: alterna o perfil.
2. Envie S, depois T: recusa com NOT_IDLE.
3. Envie X0: recusa com OUT_OF_RANGE.
4. Envie X20: aceita e reporta o novo valor.
5. Envie I: mostra o estado completo.

Teste 6 — Pino inválido (blindagem)

1. No Config.h, altere LED_RUN_PIN para 4 (mesmo pino do LED_FAULT).
2. Compile e inicie.
3. Observe: PIN_CONFIG_INVALID no boot, estado EMERGENCY, nenhum LED acende.
4. Reverta o valor.


-----

## Arquitetura

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

`IFurnace` é o único ponto de troca entre simulação e hardware real. Para migrar ao V2, basta criar **RealFurnace : public IFurnace** e trocar a declaração do objeto no main.cpp.

Assimetria de tempo (intencional)

--- | --- | ---
Domínio | Tempo | Motivo
Modelo térmico, PID, detecção de falhas | Simulado (**dt × TIME_SCALE**) | Permite acelerar a queima
SSR e LEDs | Real | Dispositivos físicos e sinalização visual

Com `TIME_SCALE = 50`, o forno esquenta 50x mais rápido, mas o LED do SSR pisca em ritmo de bancada.

---

## Configuração

Tudo em `src/Config.h`. Os dados que ocupam memória (arrays de perfil) ficam em `src/Config.cpp` e são referenciados por extern.

Limites térmicos

Constante | Valor | Significado |
--- | --- | --- |
`MAX_KILN_TEMP_C` | 1100 | Limite máximo do perfil |
`SAFETY_LIMIT_C` | 1150 | Dispara OVER_TEMPERATURE |
`MIN_VALID_TEMP_C` | -50 | Abaixo disso: leitura inválida |
`RESET_ALLOWED_C` | 200 | Reset só abaixo desta temperatura |

Pinos

Constante | Valor | Uso |
--- | --- | --- |
`SSR_PIN` | 25 | Saída do SSR |
`LED_RUN_PIN` | 2 | LED de run |
`LED_FAULT_PIN` | 4 | LED de falha |
`LED_ACK_PIN` | 13 | LED de ack (V2) — GPIO 15 evitado (strap de boot) |
`BUZZER_PIN` | 16 | Buzzer (V2) |
`RESET_BUTTON_PIN` | 32 | Botão de reset (V2) |
`ACK_BUTTON_PIN` | 33 | Botão de ack (V2) |
`THERMO_CLK_PIN` | 18 | MAX31855 CLK (V2) |
`THERMO_CS_PIN` | 5 | MAX31855 CS (V2) |
`THERMO_DO_PIN` | 19 | MAX31855 DO (V2) |

- Os botões RESET (GPIO 32) e ACK (GPIO 33) têm pull-down externo de 10k no `diagram.json`.
- A lógica do V2 deve ser **ativa em HIGH** (`pinMode(pin, INPUT)` sem `PULLUP`).
- O pull-down de 10k é mais robusto contra ruído do SSR que o pull-up interno (~45k).

Controle

Constante | Valor | Significado |
--- | --- | --- |
`SSR_WINDOW_MS` | 10000 | Janela de modulação do SSR |
`SSR_MIN_ON_MS` | 100 | Pulso mínimo confiável |
`PID_KP` / KI / KD | 2.5 / 0.02 / 15 | Ganhos do PID |
`STUCK_WINDOW_S` | 300 | Janela de confirmação de sensor travado |
`TIME_SCALE_DEFAULT` | 50 | Aceleração do tempo simulado |
`TELEMETRY_PERIOD_DEFAULT_S` | 5 | Período de telemetria (s simulados) |
`PROFILE_DEFAULT` | PROFILE_TEST | Perfil ativo na inicialização |

Limites dos comandos seriais

Constante | Valor | Significado |
--- | --- | --- |
`TIME_SCALE_MIN` | 1 | Menor escala aceita por X<n> |
`TIME_SCALE_MAX` | 500 | Maior escala aceita por X<n> |
`TELEMETRY_PERIOD_MIN` | 1 | Menor período aceito por P<n> (s) |
`TELEMETRY_PERIOD_MAX` | 600 | Maior período aceito por P<n> (s) |

Documentação adicional
- `docs/DECISOES.md` — registro de todas as decisões de projeto
- `src/Config.h` — comentários sobre cada constante
- `src/Types.h` — enums, HardwarePins e conversões para texto

---

## Pendências para o V2

- `RealFurnace` com MAX31855 (SPI)
- Reajuste dos ganhos do PID para hardware real (inércia térmica)
- Filtro na medição ou redução de `Kd` (resolução de 1 °C gera ruído)
- `SSR_STUCK_ON` com realimentação física
- `HEATING_FAILURE` com leitura real de temperatura
- Botões de reset e ack, buzzer, display
- Feedforward de rampa
- Resfriamento forçado

---

## Pontos de atenção no `diagram.json`

**Os botões estão ligados com pull-down externo implícito**. O wokwi-pushbutton fecha o contato entre `1.l` e `2.l`. Ligando `1.l` ao GPIO e `2.l` ao GND, o pino fica flutuante quando solto. No V2, use `INPUT_PULLUP` no firmware e inverta a lógica (pressionado = LOW), ou adicione um resistor de pull-down de 10k no diagrama. Recomendo `INPUT_PULLUP` — economiza componente e é o padrão do ESP32.

**O LED de ack no pino 15 é compartilhado com o strap de boot**. O GPIO 15 tem função secundária no boot do ESP32 (saída de debug do bootloader). Um LED ligado nele não impede o boot, mas pisca durante a inicialização. Se incomodar, mova para o GPIO 13 ou 14.

**Segundo ponto**: os dois botões compartilham **esp:GND.2**. O ESP32 DevKit tem dois pinos GND acessíveis na fileira direita — funciona, mas se o Wokwi reclamar de conexão duplicada, use **esp:GND.3**.


---

## Alocação de pinos — ESP32 DevKit v4 / WROOM-32

### Firmware de forno elétrico · versão V2

Mapa completo de GPIOs do firmware V2, com a origem de cada pino na V1, direção elétrica e restrições de hardware. O projeto **não usa Wi-Fi nem ADC**, então as restrições de ADC2 são inofensivas. As únicas restrições realmente críticas são os **strapping pins** e os **pinos somente-entrada** (34, 35, 36, 39).

### Tabela de alocação de pinos

19 pinos alocados · ordem de definição do `Config.h`

GPIO | Constante no Config.h | Função | Direção | Versão | Observação/Restrição
---|---|---|---|---|---
23 | `LED_RUN_PIN` | LED de run | Saída | V1 | Movido do GPIO 2 (strap de boot). Pino livre, sem ADC2, sem touch.
4 | `LED_FAULT_PIN` | LED de falha | Saída | V1 | ADC2 e touch, mas nenhum dos dois é usado no projeto.
25 | `SSR_PIN` | Saída do SSR | Saída | V1 | ADC2, não usado. Sem strap.
13 | `LED_ACK_PIN` | LED de ack de alarme | Saída | V2 | Movido do GPIO 15 (strap de boot). ADC2 e touch, não usados.
16 | `BUZZER_PIN` | Buzzer ativo | Saída | V2 | Pino livre, sem restrição.
32 | `RESET_BUTTON_PIN` | Botão de reset físico | Entrada | V2 | Touch, não usado. Pull-down externo de 10k no diagrama. Lógica ativa em HIGH.
33 | `ACK_BUTTON_PIN` | Botão de reconhecimento de alarme | Entrada | V2 | Touch, não usado. Pull-down externo de 10k. Lógica ativa em HIGH.
18 | `THERMO_CLK_PIN` | MAX31855 CLK (SPI) | Saída | V2 | SPI nativo (VSPI CLK). Sem restrição.
17 | `THERMO_CS_PIN` | MAX31855 CS (SPI) | Saída | V2 | Movido do GPIO 5 (strap de boot). Pino livre.
19 | `THERMO_DO_PIN` | MAX31855 DO / MISO (SPI) | Entrada | V2 | SPI nativo (VSPI MISO). Sem restrição.
21 | `I2C_SDA_PIN` | SDA do barramento I²C | Bidirecional | V2 | Reservado para display OLED e/ou RTC. Pino I²C padrão do ESP32.
22 | `I2C_SCL_PIN` | SCL do barramento I²C | Saída | V2 | Reservado para display OLED e/ou RTC. Pino I²C padrão do ESP32.
27 | `ENCODER_A_PIN` | Encoder rotativo canal A | Entrada | V2 | Livre. Sem strap, sem ADC2, sem touch. Suporta interrupção.
14 | `ENCODER_B_PIN` | Encoder rotativo canal B | Entrada | V2 | ADC2, não usado. Suporta interrupção.
26 | `ENCODER_BTN_PIN` | Botão do encoder (push) | Entrada | V2 | ADC2: digital ok; analógico só com Wi-Fi desligado. Movido do 14 para liberar ADC1.
34 | `ANALOG_MAINS_V_PIN` | Tensão da rede elétrica (ADC1 ch6) | Entrada apenas | V3 | SOMENTE ENTRADA, sem pull interno. Exige divisor resistivo + trafo de isolação + proteção contra surto.
35 | `ANALOG_CALIB_REF_PIN` | Referência de calibração do ADC (ADC1 ch7) | Entrada apenas | V3 | SOMENTE ENTRADA, sem pull interno. Exige referência estável (TL431), baixa tensão CC.
36 | `ANALOG_SPARE_1_PIN` | Reserva analógica (ADC1 ch0) | Entrada apenas | V3 | SOMENTE ENTRADA, sem pull interno. Mais ruidoso por proximidade do RF.
39 | `ANALOG_SPARE_2_PIN` | Reserva analógica (ADC1 ch3) | Entrada apenas | V3


### Pinos proibidos — nunca usar

Qualquer uso destes pinos compromete o boot ou o barramento

GPIO | Motivo | Consequência
---|---|---
6 a 11 | Ligados à memória flash SPI interna | Usar qualquer um deles trava ou corrompe o boot
0, 2, 5, 12, 15 | Strapping pins que definem o modo de boot | Nível errado no reset impede o boot ou entra em modo flash. GPIO 12 em HIGH configura a flash para 3,3 V e pode impedir o boot completamente
1 e 3 | UART0, usados pelo monitor serial | Conflito com a telemetria serial em 115200 baud
20, 24, 28, 29, 30, 31 | Não expostos no DevKit v4 ou reservados internamente | Inacessíveis fisicamente

### Resumo da ocupação

- 19 de 34: pinos alocados de 34 GPIOs utilizáveis
- 15 pinos livres para expansão futura


### Contexto de validação

V1 simulada · V2 com hardware real

- **V1 — validada no Wokwi** - Apenas aquecimento (SSR), LEDs de run e falha.
- **V2 — hardware real** - Termopar MAX31855 via SPI, display OLED via I²C, encoder rotativo e botões físicos.
- **ADC2 sem impacto** - O projeto não usa ADC nem Wi-Fi, então as restrições de ADC2 nos pinos 4, 25, 13, 26 e 14 são inofensivas.
- **Pinos 34, 35, 36 e 39** - Somente entrada e sem pull interno: use pull externo se precisar de nível definido.