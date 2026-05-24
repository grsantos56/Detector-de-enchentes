# FloodSec - Sistema de Deteccao de Enchentes

O **FloodSec** e um prototipo IoT de baixo custo desenvolvido para monitorar o nivel da agua em areas vulneraveis a enchentes e alagamentos. O sistema utiliza um microcontrolador ESP32, sensor ultrassonico e integracao com a plataforma Blynk para exibicao de dados em tempo real e envio de alertas em situacoes de risco.

## Objetivo

Desenvolver uma solucao acessivel para auxiliar moradores de areas de risco no acompanhamento do nivel da agua, oferecendo alertas preventivos que possam contribuir para a tomada de decisao, protecao de bens materiais e reducao da exposicao a situacoes perigosas.

## Problema

Comunidades perifericas e ribeirinhas frequentemente nao possuem acesso a sistemas de monitoramento hidrologico em tempo real. Os alertas meteorologicos gerais nem sempre refletem a velocidade com que a agua sobe em locais especificos, reduzindo o tempo de resposta dos moradores diante de enchentes.

## Funcionalidades

- Monitoramento do nivel da agua com sensor ultrassonico HC-SR04.
- Calculo da velocidade de subida da agua.
- Classificacao do risco em niveis: sem risco, leve, medio e extremo.
- Acionamento de LED fisico em niveis medio e extremo.
- Acionamento de buzzer em nivel extremo.
- Envio de dados para dashboard no Blynk.
- LED virtual no Blynk para indicacao de alerta.
- Consulta de previsao de chuva por latitude e longitude.
- Uso da previsao do tempo como fator complementar na avaliacao de risco.
- Envio de notificacao em caso de alerta extremo.

## Tecnologias Utilizadas

- ESP32
- Sensor ultrassonico HC-SR04
- LED
- Buzzer ativo 5V
- Arduino IDE
- Linguagem C/C++
- Plataforma Blynk IoT
- API Open-Meteo
- Wi-Fi
- GitHub

## Componentes

| Item | Quantidade | Valor aproximado |
|---|---:|---:|
| Modulo ESP32 DOIT DevKit | 1 | R$ 43,00 |
| Sensor Ultrassonico HC-SR04 | 2 | R$ 26,00 |
| Kit de resistores | 1 | R$ 31,00 |
| Kit de LEDs difusos 5mm | 1 | R$ 18,30 |
| Modulo buzzer ativo 5V | 1 | R$ 19,00 |
| Kit protoboard, regulador de tensao e jumpers | 1 | R$ 37,90 |
| **Total aproximado** |  | **R$ 175,20** |

## Ligacao dos Componentes

| Componente | Pino do ESP32 |
|---|---|
| TRIG do HC-SR04 | GPIO 4 |
| ECHO do HC-SR04 | GPIO 2 |
| LED | GPIO 5 |
| Buzzer | GPIO 18 |
| VCC do sensor | 5V |
| GND | GND |

> Observacao: se o sensor HC-SR04 estiver alimentado em 5V, recomenda-se usar divisor de tensao no pino ECHO ao conectar no ESP32, pois o ESP32 trabalha com logica de 3.3V.

## Arquitetura do Sistema

O projeto segue uma arquitetura IoT em tres camadas:

1. **Camada de percepcao**  
   Responsavel pela leitura fisica do ambiente usando o sensor ultrassonico HC-SR04.

2. **Camada de rede**  
   Responsavel pela conexao Wi-Fi do ESP32 e envio dos dados para a nuvem.

3. **Camada de aplicacao**  
   Responsavel pela exibicao dos dados no Blynk, visualizacao em dashboard e envio de alertas.

## Fluxo de Funcionamento

```text
Sensor HC-SR04 mede a distancia da agua
            ↓
ESP32 calcula distancia e velocidade de subida
            ↓
Sistema classifica o nivel de risco
            ↓
Dados sao enviados para o Blynk via Wi-Fi
            ↓
Dashboard exibe distancia, velocidade, clima e status
            ↓
Em caso extremo, buzzer e notificacao sao acionados
```

## Niveis de Risco

| Nivel | Condicao geral |
|---|---|
| Sem risco | Agua distante e sem aumento relevante |
| Leve | Agua em aproximacao ou subida lenta |
| Medio | Agua em nivel preocupante ou subida moderada |
| Extremo | Agua proxima do limite critico ou subida muito rapida |

O sistema tambem considera dados meteorologicos, como chance de chuva e precipitacao prevista, para tornar os alertas mais sensiveis em cenarios de maior risco.

## Configuracao do Blynk

No Blynk, crie um template para ESP32 com conexao Wi-Fi e configure os seguintes Datastreams:

| Pino virtual | Nome | Tipo |
|---|---|---|
| V0 | Distancia | Double |
| V1 | Velocidade de subida | Double |
| V2 | Nivel de risco | String |
| V3 | Chance de chuva | Double |
| V4 | Precipitacao prevista | Double |
| V5 | LED Alerta | Integer |

Tambem crie um evento de notificacao:

```text
alerta_extremo
```

## Configuracao do Codigo

Antes de enviar o codigo para o ESP32, preencha os dados abaixo:

```cpp
#define BLYNK_TEMPLATE_ID "SEU_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Detector de Enchentes"
#define BLYNK_AUTH_TOKEN "SEU_AUTH_TOKEN"

char ssid[] = "NOME_DO_WIFI";
char pass[] = "SENHA_DO_WIFI";

const float LATITUDE = SUA_LATITUDE;
const float LONGITUDE = SUA_LONGITUDE;
```

## Dashboard Recomendado

No Blynk, recomenda-se usar os seguintes widgets:

- Gauge para distancia da agua.
- Gauge ou label para velocidade de subida.
- Label para nivel de risco.
- Gauge para chance de chuva.
- Label ou gauge para precipitacao prevista.
- LED virtual para indicacao de alerta.

## Resultados Esperados

Espera-se que o prototipo seja capaz de realizar leituras constantes do nivel da agua, transmitir os dados para a plataforma Blynk e emitir alertas preventivos em situacoes de risco. A solucao busca demonstrar como tecnologias acessiveis podem ser aplicadas na prevencao de desastres naturais e no apoio a comunidades vulneraveis.

## Possiveis Melhorias Futuras

- Uso de sensores de pressao ou sensores de nivel submersiveis.
- Adicao de pluviometro para medir chuva local.
- Integracao com sensores de temperatura e umidade.
- Uso de LoRa ou NB-IoT para maior alcance.
- Armazenamento historico dos dados em banco de dados.
- Aplicacao de aprendizado de maquina para previsao de enchentes.
- Integracao com sistemas oficiais de alerta e Defesa Civil.

## Equipe

- Herick Ruan Silva Viana
- Joel de Jesus Felix Lima
- Kevin Leonardo Silva da Sousa
- Gabriel Rodrigues dos Santos
- Wandrew Ricardo da Rocha Oliveira
- Samuel Salustiano Oliveira Martins
- Felipe da Silva Araujo
- Guilherme Ruan Gomes Carvalho

## Referencias

TUCCI, Carlos E. M. **Gestao de aguas pluviais urbanas**. Brasilia: Ministerio das Cidades, 2005.

TUNDISI, Jose Galizia; TUNDISI, Takako Matsumura. **Limnologia**. Sao Paulo: Oficina de Textos, 2008.

FAHAD, Shahzada. **IoT based flood monitoring system using ultrasonic sensor and ESP8266**. 2024. Disponivel em: https://www.electroniclinic.com/iot-based-flood-monitoring-system-using-ultrasonic-sensor-and-esp8266/

CUADRA, Roy. **IoT-based flood detection system**. 2025. Disponivel em: https://github.com/roycuadra/IoT-based-flood-detection-system
