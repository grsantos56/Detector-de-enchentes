#define BLYNK_TEMPLATE_ID "TMPL2ELT0sGQm"
#define BLYNK_TEMPLATE_NAME "Detector de Enchentes"
#define BLYNK_AUTH_TOKEN "RIcRbMjTSvOf5qT_guQlhj_RF67__CNC"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Casa_2.4";
char pass[] = "misquici";

const float LATITUDE = -5.0892;
const float LONGITUDE = -42.8019;
const char CIDADE_MONITORADA[] = "Teresina - PI";

const int PINO_TRIG = 4;
const int PINO_ECHO = 2;

const int PINO_LED = 5;
const int PINO_BUZZER = 18;

const bool MODO_TESTE_VASILHA = true;

const float DISTANCIA_ALERTA_LEVE_CM = 15.0;
const float DISTANCIA_ALERTA_MEDIO_CM = 10.0;
const float DISTANCIA_ALERTA_EXTREMO_CM = 5.0;

const float DISTANCIA_RECUPERA_LEVE_CM = 17.0;
const float DISTANCIA_RECUPERA_MEDIO_CM = 12.0;
const float DISTANCIA_RECUPERA_EXTREMO_CM = 7.0;
const float DISTANCIA_MAXIMA_TESTE_CM = 35.0;

const float VELOCIDADE_ALERTA_LEVE_CM_S = 0.02;
const float VELOCIDADE_ALERTA_MEDIO_CM_S = 0.08;
const float VELOCIDADE_ALERTA_EXTREMO_CM_S = 0.17;
const float VELOCIDADE_MAXIMA_TESTE_CM_MIN = 20.0;
const float VELOCIDADE_MAXIMA_TESTE_CM_S = VELOCIDADE_MAXIMA_TESTE_CM_MIN / 60.0;
const float VELOCIDADE_MINIMA_EXIBICAO_CM_MIN = 0.05;

const int TOTAL_LEITURAS = 5;
const float PESO_FILTRO_VELOCIDADE = 0.35;
const float VARIACAO_MINIMA_CM = 0.2;
const unsigned long INTERVALO_VELOCIDADE_MS = 2UL * 1000UL;
const unsigned long TEMPO_SEM_SUBIR_MS = 10UL * 60UL * 1000UL;
const unsigned long INTERVALO_CLIMA_MS = 15UL * 60UL * 1000UL;
const unsigned long INTERVALO_BLYNK_MS = 100;
const unsigned long INTERVALO_NOTIFICACAO_MS = 10UL * 60UL * 1000UL;

float distanciaReferenciaVelocidade = -1.0;
float distanciaAtual = -1.0;
float velocidadeFiltrada = 0.0;
float chanceChuva = 0.0;
float chuvaPrevistaMm = 0.0;
unsigned long tempoReferenciaVelocidade = 0;
unsigned long ultimaSubidaAgua = 0;
unsigned long ultimaNotificacaoExtremo = 0;
bool buzzerLigado = false;
int ledBlynk = 0;
String nivelAtualTexto = "SEM RISCO";

BlynkTimer timer;

enum NivelAlerta {
  SEM_ALERTA,
  ALERTA_LEVE,
  ALERTA_MEDIO,
  ALERTA_EXTREMO
};

enum RiscoClima {
  CLIMA_NORMAL,
  CLIMA_ATENCAO,
  CLIMA_PERIGOSO
};

NivelAlerta nivelComMemoria = SEM_ALERTA;

float medirDistanciaCm() {
  float somaDistancias = 0.0;
  int leiturasValidas = 0;

  for (int i = 0; i < TOTAL_LEITURAS; i++) {
    digitalWrite(PINO_TRIG, LOW);
    delayMicroseconds(2);

    digitalWrite(PINO_TRIG, HIGH);
    delayMicroseconds(10);

    digitalWrite(PINO_TRIG, LOW);

    long duracao = pulseIn(PINO_ECHO, HIGH, 30000);

    if (duracao > 0) {
      somaDistancias += (duracao * 0.0343) / 2;
      leiturasValidas++;
    }

    delay(40);
  }

  if (leiturasValidas == 0) {
    return -1.0;
  }

  return somaDistancias / leiturasValidas;
}

RiscoClima calcularRiscoClima() {
  if (chanceChuva >= 90.0 || chuvaPrevistaMm >= 15.0) {
    return CLIMA_PERIGOSO;
  }

  if (chanceChuva >= 70.0 || chuvaPrevistaMm >= 5.0) {
    return CLIMA_ATENCAO;
  }

  return CLIMA_NORMAL;
}

NivelAlerta calcularNivelAlerta(float distancia, float velocidadeAproximacao) {
  RiscoClima riscoClima = calcularRiscoClima();
  float ajusteClima = 0.0;

  if (MODO_TESTE_VASILHA) {
    ajusteClima = 0.0;
  } else if (riscoClima == CLIMA_ATENCAO) {
    ajusteClima = 5.0;
  } else if (riscoClima == CLIMA_PERIGOSO) {
    ajusteClima = 10.0;
  }

  float limiteLeve = DISTANCIA_ALERTA_LEVE_CM + ajusteClima;
  float limiteMedio = DISTANCIA_ALERTA_MEDIO_CM + (ajusteClima / 2.0);
  float limiteExtremo = DISTANCIA_ALERTA_EXTREMO_CM;
  float velocidadeMedio = VELOCIDADE_ALERTA_MEDIO_CM_S;
  float velocidadeExtremo = VELOCIDADE_ALERTA_EXTREMO_CM_S;

  if (!MODO_TESTE_VASILHA && riscoClima == CLIMA_PERIGOSO) {
    velocidadeMedio = 0.06;
    velocidadeExtremo = 0.12;
  }

  if (distancia <= limiteExtremo ||
      (distancia <= limiteMedio &&
       velocidadeAproximacao >= velocidadeExtremo)) {
    return ALERTA_EXTREMO;
  }

  if (distancia <= limiteMedio ||
      (distancia <= limiteLeve &&
       velocidadeAproximacao >= velocidadeMedio)) {
    return ALERTA_MEDIO;
  }

  if (distancia <= limiteLeve ||
      velocidadeAproximacao >= VELOCIDADE_ALERTA_LEVE_CM_S) {
    return ALERTA_LEVE;
  }

  return SEM_ALERTA;
}

int prioridadeNivel(NivelAlerta nivel) {
  if (nivel == ALERTA_EXTREMO) {
    return 3;
  }

  if (nivel == ALERTA_MEDIO) {
    return 2;
  }

  if (nivel == ALERTA_LEVE) {
    return 1;
  }

  return 0;
}

bool aguaRecuouParaReduzirNivel(NivelAlerta nivel, float distancia) {
  if (nivel == ALERTA_EXTREMO) {
    return distancia >= DISTANCIA_RECUPERA_EXTREMO_CM;
  }

  if (nivel == ALERTA_MEDIO) {
    return distancia >= DISTANCIA_RECUPERA_MEDIO_CM;
  }

  if (nivel == ALERTA_LEVE) {
    return distancia >= DISTANCIA_RECUPERA_LEVE_CM;
  }

  return true;
}

NivelAlerta atualizarNivelComMemoria(NivelAlerta nivelAtual, NivelAlerta nivelCalculado, float distancia) {
  if (prioridadeNivel(nivelCalculado) > prioridadeNivel(nivelAtual)) {
    ultimaSubidaAgua = millis();
    return nivelCalculado;
  }

  if (nivelCalculado == nivelAtual) {
    return nivelAtual;
  }

  bool aguaRecuou = aguaRecuouParaReduzirNivel(nivelAtual, distancia);
  bool ficouTempoSemSubir = millis() - ultimaSubidaAgua >= TEMPO_SEM_SUBIR_MS;

  if (prioridadeNivel(nivelCalculado) < prioridadeNivel(nivelAtual) &&
      (aguaRecuou || ficouTempoSemSubir)) {
    return nivelCalculado;
  }

  return nivelAtual;
}

void atualizarBuzzer(NivelAlerta nivel) {
  if (nivel != ALERTA_EXTREMO) {
    digitalWrite(PINO_BUZZER, HIGH);
    buzzerLigado = false;
    return;
  }

  digitalWrite(PINO_BUZZER, LOW);
  buzzerLigado = true;
}

const char* nomeNivelAlerta(NivelAlerta nivel) {
  if (nivel == ALERTA_EXTREMO) {
    return "EXTREMO";
  }

  if (nivel == ALERTA_MEDIO) {
    return "MEDIO";
  }

  if (nivel == ALERTA_LEVE) {
    return "LEVE";
  }

  return "SEM RISCO";
}

void atualizarClima() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(LATITUDE, 4);
  url += "&longitude=";
  url += String(LONGITUDE, 4);
  url += "&hourly=precipitation_probability,precipitation&forecast_days=1&timezone=auto";

  http.begin(client, url);
  int codigoHttp = http.GET();

  if (codigoHttp == 200) {
    String resposta = http.getString();
    DynamicJsonDocument doc(12000);
    DeserializationError erro = deserializeJson(doc, resposta);

    if (!erro) {
      chanceChuva = doc["hourly"]["precipitation_probability"][0] | 0.0;
      chuvaPrevistaMm = doc["hourly"]["precipitation"][0] | 0.0;
    }
  }

  http.end();
}

float velocidadeParaBlynkCmMin() {
  float velocidadeCmMin = velocidadeFiltrada * 60.0;

  if (velocidadeCmMin < VELOCIDADE_MINIMA_EXIBICAO_CM_MIN) {
    return 0.0;
  }

  if (MODO_TESTE_VASILHA && velocidadeCmMin > VELOCIDADE_MAXIMA_TESTE_CM_MIN) {
    return VELOCIDADE_MAXIMA_TESTE_CM_MIN;
  }

  return velocidadeCmMin;
}

void enviarDadosBlynk() {
  Blynk.virtualWrite(V0, distanciaAtual);
  Blynk.virtualWrite(V1, velocidadeParaBlynkCmMin());
  Blynk.virtualWrite(V2, nivelAtualTexto);
  Blynk.virtualWrite(V3, chanceChuva);
  Blynk.virtualWrite(V4, chuvaPrevistaMm);
  Blynk.virtualWrite(V5, ledBlynk);
  Blynk.virtualWrite(V6, CIDADE_MONITORADA);
}

void notificarBlynkSeExtremo(NivelAlerta nivel) {
  if (nivel != ALERTA_EXTREMO) {
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaNotificacaoExtremo < INTERVALO_NOTIFICACAO_MS) {
    return;
  }

  String mensagem = "Alerta EXTREMO: distancia ";
  mensagem += String(distanciaAtual, 1);
  mensagem += " cm, chuva ";
  mensagem += String(chanceChuva, 0);
  mensagem += "%";

  Blynk.logEvent("alerta_extremo", mensagem);
  ultimaNotificacaoExtremo = agora;
}

void setup() {

  Serial.begin(9600);

  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  // buzzer comeca desligado
  digitalWrite(PINO_BUZZER, HIGH);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  atualizarClima();
  timer.setInterval(INTERVALO_CLIMA_MS, atualizarClima);
  timer.setInterval(INTERVALO_BLYNK_MS, enviarDadosBlynk);
}

void loop() {
  Blynk.run();
  timer.run();

  // mede distancia
  float distancia = medirDistanciaCm();

  if (distancia < 0) {
    Serial.println("Sem risco");
    digitalWrite(PINO_LED, LOW);
    digitalWrite(PINO_BUZZER, HIGH);
    nivelAtualTexto = "SEM RISCO";
    nivelComMemoria = SEM_ALERTA;
    ledBlynk = 0;
    delay(500);
    return;
  }

  if (MODO_TESTE_VASILHA && distancia > DISTANCIA_MAXIMA_TESTE_CM) {
    distancia = DISTANCIA_MAXIMA_TESTE_CM;
    distanciaReferenciaVelocidade = distancia;
    tempoReferenciaVelocidade = millis();
    velocidadeFiltrada = 0.0;
  }

  distanciaAtual = distancia;

  unsigned long tempoAtual = millis();
  float velocidadeAproximacao = 0.0;

  if (distanciaReferenciaVelocidade < 0 || tempoReferenciaVelocidade == 0) {
    distanciaReferenciaVelocidade = distancia;
    tempoReferenciaVelocidade = tempoAtual;
    ultimaSubidaAgua = tempoAtual;
  }

  if (tempoAtual - tempoReferenciaVelocidade >= INTERVALO_VELOCIDADE_MS) {
    float tempoSegundos = (tempoAtual - tempoReferenciaVelocidade) / 1000.0;
    float variacaoDistancia = distanciaReferenciaVelocidade - distancia;

    if (tempoSegundos > 0) {
      if (variacaoDistancia >= VARIACAO_MINIMA_CM) {
        velocidadeAproximacao = variacaoDistancia / tempoSegundos;
        ultimaSubidaAgua = tempoAtual;
      }

      if (MODO_TESTE_VASILHA && velocidadeAproximacao > VELOCIDADE_MAXIMA_TESTE_CM_S) {
        velocidadeAproximacao = VELOCIDADE_MAXIMA_TESTE_CM_S;
      }

      velocidadeFiltrada = (PESO_FILTRO_VELOCIDADE * velocidadeAproximacao) +
                           ((1.0 - PESO_FILTRO_VELOCIDADE) * velocidadeFiltrada);
    }

    distanciaReferenciaVelocidade = distancia;
    tempoReferenciaVelocidade = tempoAtual;
  }

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.print(" cm | Velocidade de aproximacao: ");
  Serial.print(velocidadeParaBlynkCmMin());
  Serial.print(" cm/min | Nivel: ");
  NivelAlerta nivelCalculado = calcularNivelAlerta(distancia, velocidadeFiltrada);
  NivelAlerta nivel = atualizarNivelComMemoria(nivelComMemoria, nivelCalculado, distancia);
  nivelComMemoria = nivel;
  nivelAtualTexto = nomeNivelAlerta(nivel);
  Serial.print(nivelAtualTexto);
  Serial.print(" | Chuva: ");
  Serial.print(chanceChuva);
  Serial.print("% | Precipitacao: ");
  Serial.print(chuvaPrevistaMm);
  Serial.println(" mm");

  if (nivel == ALERTA_MEDIO || nivel == ALERTA_EXTREMO) {
    digitalWrite(PINO_LED, HIGH);
    ledBlynk = 255;
  } else {
    digitalWrite(PINO_LED, LOW);
    ledBlynk = 0;
  }

  atualizarBuzzer(nivel);
  notificarBlynkSeExtremo(nivel);

  delay(50);
}
