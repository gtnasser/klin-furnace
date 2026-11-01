# KLIN-FURNACE 

Controlador de forno elétrico por perfil de queima, com PID, detecção de falhas e telemetria serial. Firmware em C++ para ESP32.

## 1. Decisões tomadas

Plataforma e escopo

- V1 roda no simulador Wokwi, com ESP32 e Arduino core
- V1 é apenas aquecimento; resfriamento forçado fica para versão futura, mas a arquitetura prevê o encaixe
- V2 roda na bancada com termopar real
- Termopar adotado: MAX31855 (SPI), por ter detecção de falha embutida
- Resolução de temperatura: 1 °C em todo o sistema

Arquitetura

- Objetos globais estáticos no main.cpp + injeção por ponteiro no construtor do KilnController
- Config.h mantém #define para o fixo e extern para as três globais mutáveis (g_timeScale, g_telemetryPeriodS, g_activeProfile)
- Perfis como const float [][2] no header, com cópia interna no KilnProfile
- ProfileId como enum : uint8_t
- INVALID_PROFILE saiu do FaultCode e virou ProfileStatus, retornado por validate()
- FaultDetector::update() recebe setpoint e dt
- SENSOR_STUCK exige três condições simultâneas: aquecendo, plano e longe do alvo
- SSR_STUCK_ON e HEATING_FAILURE ficam declarados no V1 sem geração, com comentário explícito
- SimulatedFurnace recebe potência média do PID, não o binário do SSR

Configuração e operação

- TIME_SCALE e período de telemetria são parâmetros mutáveis, não constantes
- Perfil ativo: padrão por compilação, sobrescrita em runtime
- Dois perfis: PRODUCTION (435 min) e TEST (10 min)
- Telemetria em JSON por linha, emitida no fim do ciclo do controlador
- Frequência de telemetria contada em tempo simulado
- Interface do V1: serial no firmware + ponte serial→navegador fora do firmware
- PID com derivada sobre a medição (evita tranco nas transições de segmento) e anti-windup duplo (clamping + desfazer incremento na saturação).
- shutdown() corta potência mas não zera temperatura; o modelo só evolui enquanto setPower é chamado.
- Modelo de primeira ordem sem inércia térmica — ganhos do PID precisarão de reajuste no V2.


Hardware do V1

- Sem botões, sem buzzer no V1
- LEDs de run e fault implementados; LED de ack reservado
- Pinos do termopar definidos no Config.h mesmo sem uso


## 2. Módulos e responsabilidades

Módulo | Responsabilidade| Depende de
--- | --- | ---
Config.h | Constantes de hardware, segurança, PID, falhas, perfis e globais mutáveis | —
Types.h | Enums de estado, falha, alarme e status de perfil; conversões para texto | Arduino.h
KilnProfile | Armazenar pontos, validar perfil, interpolar setpoint, informar segmento e fim } Types.h, Config.h
FaultDetector | Avaliar temperatura, setpoint, potência e SSR; devolver código de falha | Types.h, Config.h
PIDController | Calcular potência 0–100% a partir de erro, com anti-windup | Config.h
SSRDriver | Converter potência em pulsos de tempo sobre o pino do SSR | Config.h
IFurnace | Interface abstrata do forno: ler temperatura, aplicar potência | —
SimulatedFurnace | Implementar IFurnace com modelo térmico | IFurnace, Config.h
KilnController | Orquestrar tudo, aplicar escala de tempo, emitir telemetria | Todos os acima
Telemetry | Montar e emitir o JSON de telemetria | Types.h, Config.h
StatusLeds | Refletir estado e falha nos LEDs | Types.h, Config.h
main.cpp | Definir globais, instanciar objetos, setup() e loop() | Todos

Fluxo de dependência: 
```text
main.cpp → KilnController → (KilnProfile, FaultDetector, PIDController, SSRDriver, IFurnace, Telemetry, StatusLeds).
```

- Nenhum módulo de baixo nível conhece o controlador. 
- IFurnace é o único ponto de troca entre simulação e hardware real.

## 3. FUNCIONALIDADES da V1

Basicamente: controle de aquecimento com PID, validação do perfil de queima, quatro falhas ativas e telemetria JSON.

Controle

- Máquina de estados com 6 estados: IDLE, STARTING, RUNNING, FINISHED, RESETTING, EMERGENCY
- Perfil de queima por pontos com interpolação linear
- Validação de perfil com motivo específico de rejeição
- PID com anti-windup e limite de integral
- Acionamento do SSR por time-proportioning em janela de 10 s
- Aplicação de TIME_SCALE ao delta de tempo
- Detecção de fim de perfil e transição para FINISHED

Segurança

- Limite de perfil em 1100 °C
- Limite de segurança em 1150 °C, com entrada em EMERGENCY
- Reset permitido apenas abaixo de 200 °C
- Detecção de termopar aberto (leitura inválida)
- Detecção de sensor travado com consciência de setpoint
- Detecção de sobretemperatura
- SSR forçado a desligar em emergência

Observabilidade

- Telemetria JSON por linha com 10 campos
- LEDs de run e fault
- Saída serial sempre ativa

Simulação

- Modelo térmico com potência média
- Dois perfis selecionáveis
- Aceleração de tempo configurável
