# Estudo de Alimentação por Bateria de Lítio

> **Nota**: primeira sessão do estudo. Objetivo: viabilizar
> alimentar o circuito por uma bateria de Li-ion de 2500mAh, com leitura
> de tensão da bateria pelo ADC do ESP32-S3, e estimar quantos dias a
> bateria duraria. Discussão ainda em andamento -- este documento registra
> o que já foi medido/decidido para continuar em outra sessão.

## Objetivo

1. Definir um circuito de leitura de tensão da bateria (divisor resistivo
   num pino ADC livre).
2. Estimar a duração de uma bateria de 2500mAh alimentando o projeto,
   dado o padrão de wake a cada `DEEP_SLEEP_INTERVAL_SEC` (60s, ver
   `config.h`) com sync de WiFi/NTP só quando `NTP_RESYNC_INTERVAL_MS`
   vence (1h).

## Contexto da placa

- Board real: **ESP32-S3-DevKitC-1**, sem circuito de carga/BMS onboard
  (não é um board tipo Feather/LiPo).
- Bridge USB-serial onboard é um chip **WCH** ("USB Single Serial",
  VID:PID `1A86:55D3`), aparece como `/dev/ttyACM0` no Linux -- não é um
  CH340 clássico, mas mesma categoria (chip separado do ESP32-S3,
  alimentado pelo mesmo trilho de 3.3V/5V USB).
- Pinos ADC1 livres nesta placa (evitando os já usados: display
  4/10/11/12/16/17; sensores/botão 1/2/6/8/9; ver `Pin Mapping.md`):
  candidatos incluem **GPIO5** (recomendado) e GPIO7. ADC2 (GPIO11-20)
  foi descartado porque compartilha circuito com o rádio WiFi, que este
  projeto usa ativamente antes de dormir.

## Medições da primeira sessão

### Corrente (medidor do usuário, resolução de 10mA, inline USB-C entre PC e a DevKit)

| Estado | Corrente |
|---|---|
| WiFi ligado | ~0,2 A |
| Atualização normal (sem WiFi, a cada 60s) | ~0,03 A |
| Deep sleep | <0,01 A (piso de resolução do medidor -- não sabemos o valor real) |

Importante: essa medição é *inline no cabo USB* (lado 5V/VBUS, antes do
LDO onboard), então o valor de sleep já inclui LDO + chip USB-serial +
ESP32-S3 somados. Isso descarta o cenário pessimista de 10-15mA de fuga
só da placa (que eu tinha levantado como hipótese antes de medir), mas
não fecha o valor exato -- ainda falta resolução abaixo de 10mA.

### Tempo de cada fase (instrumentação `millis()` adicionada temporariamente em `src/main.cpp` para esta medição, depois revertida)

Adicionados três prints `[TIMING]` (gated pelas macros `DEBUG_PRINTLN`
existentes em `debug.h`, custo zero quando `APP_DEBUG_SERIAL=0`, o padrão
de produção). Testado gravando com
`PLATFORMIO_BUILD_FLAGS=-DAPP_DEBUG_SERIAL=1 pio run -t upload` e lendo
`/dev/ttyACM0` via pyserial (o `pio device monitor` normal precisa de um
TTY interativo real, que não está disponível neste ambiente de agente).

| Fase | Duração medida |
|---|---|
| Wake normal, sem WiFi | **5.600 ms** |
| Fase WiFi (conexão + NTP + fetch clima/notícias/cripto) | **10.026 ms** |
| Wake completo com WiFi (total) | 15.626 ms (= 10.026 + 5.600, bate exato) |
| Draw phase (refresh completo do e-paper, dentro dos 5.600ms acima) | ~5.480 ms |

### Conta de energia ativa por hora

Com 59 wakes normais + 1 wake com WiFi por hora, usando as correntes
medidas pelo usuário (30mA / 200mA):

- Normal: 59 × (30mA × 5,6s / 3600) ≈ 2,76 mAh
- WiFi: 1 × [(200mA × 10,03s/3600) + (30mA × 5,6s/3600)] ≈ 0,60 mAh
- **Total ativo: ~3,36 mAh/hora**

Bem mais barato do que a estimativa inicial (~10,7mAh/h, baseada em
chutes de duração antes de medir).

### Estimativa de duração da bateria (2500mAh), por hipótese de sleep

| Sleep (hipótese) | Total/hora | Duração |
|---|---|---|
| 10mA (pior caso, limite do medidor) | ~13,3 mAh/h | ~7,8 dias |
| 1mA (plausível pra devkit) | ~4,36 mAh/h | ~24 dias |
| 0,5mA (bom caso) | ~3,88 mAh/h | ~27 dias |

O gargalo do cálculo final é só o valor real do sleep -- a parte ativa já
está fechada com números medidos.

## Achado extra: wakes de ruído do AS3935

No teste de bancada, o sensor de raios (AS3935) disparou vários wakes por
IRQ de "disturber" (ruído, não raio confirmado) -- ~5-6 em ~75s, bem mais
frequente que o wake de 60s do timer. Cada um é curto (não chega a ligar
display/WiFi), mas se essa taxa se repetir na instalação final (a
bancada tem bastante ruído eletromagnético de PC/monitor por perto,
pode não representar o ambiente real), a energia somada desses wakes
extras não está contabilizada na tabela acima. Não instrumentado ainda.

## Discussão anterior descartada (ou não): CH340/LDO

Antes de medir, consideramos modificar a placa pra desligar o
regulador/chip USB-serial durante o sleep (via chave reversível), pra
cortar uma fuga hipotética de vários mA. Como a medição via USB já
mostrou que o sistema inteiro (LDO+USB-serial+ESP32-S3) fica abaixo de
10mA em sleep, essa modificação invasiva provavelmente não é necessária
-- fica de lado a menos que o valor real do sleep (ainda não medido)
apareça alto.

## Próximos passos

1. **Medir o sleep abaixo de 10mA**: checar se o multímetro do usuário
   tem faixa de µA separada; senão, método do capacitor (carrega um
   capacitor grande, desconecta, cronometra a queda de tensão pelo
   voltímetro) ou um módulo INA219/INA226.
2. Instrumentar e medir a taxa/duração real dos wakes de ruído do AS3935
   na instalação final (não só na bancada).
3. Fechar a estimativa de dias de bateria com o valor real de sleep.
4. Desenhar o circuito de leitura de tensão: divisor resistivo no
   GPIO5 (ADC1), decidir se vale um MOSFET de alto lado pra cortar a
   fuga do divisor durante o sleep, e decidir a abordagem de carregador.

## Estado do código

A instrumentação `[TIMING]` (3 linhas de `millis()`) usada para medir as
fases acima já foi revertida do working tree -- `src/main.cpp` não tem
mais essas linhas. Reintroduzi-la (custo zero em produção, gated pelas
mesmas macros `DEBUG_PRINTLN`) é um passo rápido se a medição precisar
ser reproduzida.
