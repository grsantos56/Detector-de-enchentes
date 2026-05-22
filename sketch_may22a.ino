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

const float LATITUDE = -23.5505;
const float LONGITUDE = -46.6333;

const int PINO_TRIG = 4;
const int PINO_ECHO = 2;

const int PINO_LED = 5;
const int PINO_BUZZER = 18;

const float DISTANCIA_ALERTA_LEVE_CM = 30.0;
const float DISTANCIA_ALERTA_MEDIO_CM = 15.0;
const float DISTANCIA_ALERTA_EXTREMO_CM = 3.0;

const float VELOCIDADE_ALERTA_LEVE_CM_S = 2.0;
const float VELOCIDADE_ALERTA_MEDIO_CM_S = 6.0;
const float VELOCIDADE_ALERTA_EXTREMO_CM_S = 15.0;

const int TOTAL_LEITURAS = 5;
const float PESO_FILTRO_VELOCIDADE = 0.35;
const float VARIACAO_MINIMA_CM = 1.0;
const unsigned long INTERVALO_CLIMA_MS = 15UL * 60UL * 1000UL;
const unsigned long INTERVALO_BLYNK_MS = 2000;
const unsigned long INTERVALO_NOTIFICACAO_MS = 10UL * 60UL * 1000UL;

float distanciaAnterior = -1.0;
float distanciaAtual = -1.0;
float velocidadeFiltrada = 0.0;
float chanceChuva = 0.0;
float chuvaPrevistaMm = 0.0;
unsigned long tempoAnterior = 0;
unsigned long ultimaNotificacaoExtremo = 0;
bool buzzerLigado = false;
String nivelAtualTexto = "SEM ALERTA";

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

  if (riscoClima == CLIMA_ATENCAO) {
    ajusteClima = 5.0;
  } else if (riscoClima == CLIMA_PERIGOSO) {
    ajusteClima = 10.0;
  }

  float limiteLeve = DISTANCIA_ALERTA_LEVE_CM + ajusteClima;
  float limiteMedio = DISTANCIA_ALERTA_MEDIO_CM + (ajusteClima / 2.0);
  float limiteExtremo = DISTANCIA_ALERTA_EXTREMO_CM;
  float velocidadeMedio = VELOCIDADE_ALERTA_MEDIO_CM_S;
  float velocidadeExtremo = VELOCIDADE_ALERTA_EXTREMO_CM_S;

  if (riscoClima == CLIMA_PERIGOSO) {
    velocidadeMedio = 4.0;
    velocidadeExtremo = 10.0;
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

  return "SEM ALERTA";
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

void enviarDadosBlynk() {
  Blynk.virtualWrite(V0, distanciaAtual);
  Blynk.virtualWrite(V1, velocidadeFiltrada);
  Blynk.virtualWrite(V2, nivelAtualTexto);
  Blynk.virtualWrite(V3, chanceChuva);
  Blynk.virtualWrite(V4, chuvaPrevistaMm);
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
  distanciaAtual = distancia;

  if (distancia < 0) {
    Serial.println("Sem leitura do sensor");
    digitalWrite(PINO_LED, LOW);
    digitalWrite(PINO_BUZZER, HIGH);
    delay(500);
    return;
  }

  unsigned long tempoAtual = millis();
  float velocidadeAproximacao = 0.0;

  if (distanciaAnterior > 0 && tempoAnterior > 0) {
    float tempoSegundos = (tempoAtual - tempoAnterior) / 1000.0;
    float variacaoDistancia = distanciaAnterior - distancia;

    if (tempoSegundos > 0) {
      if (variacaoDistancia >= VARIACAO_MINIMA_CM) {
        velocidadeAproximacao = variacaoDistancia / tempoSegundos;
      }

      velocidadeFiltrada = (PESO_FILTRO_VELOCIDADE * velocidadeAproximacao) +
                           ((1.0 - PESO_FILTRO_VELOCIDADE) * velocidadeFiltrada);
    }
  }

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.print(" cm | Velocidade de aproximacao: ");
  Serial.print(velocidadeFiltrada);
  Serial.print(" cm/s | Nivel: ");
  NivelAlerta nivel = calcularNivelAlerta(distancia, velocidadeFiltrada);
  nivelAtualTexto = nomeNivelAlerta(nivel);
  Serial.print(nivelAtualTexto);
  Serial.print(" | Chuva: ");
  Serial.print(chanceChuva);
  Serial.print("% | Precipitacao: ");
  Serial.print(chuvaPrevistaMm);
  Serial.println(" mm");

  if (nivel == ALERTA_MEDIO || nivel == ALERTA_EXTREMO) {
    digitalWrite(PINO_LED, HIGH);
  } else {
    digitalWrite(PINO_LED, LOW);
  }

  atualizarBuzzer(nivel);
  notificarBlynkSeExtremo(nivel);

  distanciaAnterior = distancia;
  tempoAnterior = tempoAtual;

  delay(50);
}
