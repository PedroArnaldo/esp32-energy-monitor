/*
 * ============================================================
 *  TESTE DE VALIDACAO DO CIRCUITO + FILTRO RMS
 *  Projeto IC - Monitoramento Energetico
 * ============================================================
 *
 *  Este codigo testa o circuito de condicionamento (divisor + capacitor)
 *  e agora utiliza o calculo RMS para extrair o sinal AC limpo.
 *
 *  Vai executar 3 testes em sequencia:
 *    1. Offset DC (verifica se esta em ~1,65V)
 *    2. Ruido (verifica estabilidade)
 *    3. Detecao de sinal (verifica se responde ao sensor com RMS)
 */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"

// ============================================================
//  CONFIGURACAO DE REDE E API
// ============================================================
const char* WIFI_SSID_STR = WIFI_SSID;
const char* WIFI_PASS_STR = WIFI_PASS;
const char* API_URL       = "http://" API_HOST ":8080/api/sensors/save";

// Identificacao desta unidade (vai pro banco)
const char* EQUIPAMENTO = "Computador - 800w";
const char* LOCALIZACAO = "Residencia - Sala";
const char* UNIDADE     = "A";

// ============================================================
//  CALIBRACAO -- AJUSTE COM CARGA CONHECIDA NA BANCADA
// ============================================================
const float AMPS_POR_VOLT = 30.0;   // SCT-013-030: 1V = 30A
const float TENSAO_REDE   = 127.0;  // 127 ou 220, conforme sua rede
const float LIMIAR_RUIDO  = 0.030;  // abaixo de 30mV RMS = considera zero

// ============================================================
//  AGREGACAO -- amostra a cada 2s, envia a media a cada 30s
// ============================================================
const int AMOSTRAS_POR_ENVIO = 15;

float somaCorrente = 0;
int   contadorAmostras = 0;

// ============================================================
//  PARAMETROS
// ============================================================
const int PINO_ADC = 4;         // GPIO 4 = ADC10

const int   NUM_AMOSTRAS_TESTE = 500;   // Amostras por teste
const float VREF               = 3.3;   // Tensao de referencia
const int   ADC_MAX            = 4095;  // 12 bits

// Faixas de aceitacao (o que consideramos "OK")
const int   OFFSET_ESPERADO       = 2048;  // Meio da escala = 1,65V
const int   OFFSET_TOLERANCIA     = 250;   // +/- desse valor
const int   RUIDO_MAXIMO_OK       = 50;    // Variacao aceitavel

// ============================================================
//  ESTRUTURAS
// ============================================================
// Agrupa o resultado da amostragem, incluindo agora o valor RMS
struct AmostragemStats {
  int   minimo;
  int   maximo;
  int   variacao;   // maximo - minimo
  float media;
  float rmsTensao;  // Valor RMS convertido para Volts
};

// ============================================================
//  PROTOTIPOS
// ============================================================
void teste1_offset();
void teste2_ruido();
void teste3_deteccaoSinal();

AmostragemStats amostrarADC(int pino, int numAmostras, int atrasoMicros = 200);
float adcParaTensao(float valorADC);
void  separador(char c);
void conectarWiFi();
void sincronizarRelogio();
bool enviarLeitura(float correnteA, float potenciaVA);
String timestampISO8601();
void imprimirLeitura(float correnteA, float potenciaVA);

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);  // Espera Serial estabilizar

  // Configura ADC pra usar 12 bits e faixa ate 3,3V
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println();
  separador('=');
  Serial.println(" TESTE DE VALIDACAO - Circuito de Offset SCT-013");
  separador('=');
  Serial.println();
  Serial.println("Aguardando 3 segundos para estabilizacao...");
  Serial.println("(o capacitor precisa carregar completamente)");
  delay(3000);

  // Executa os 3 testes em sequencia
  teste1_offset();
  teste2_ruido();
  teste3_deteccaoSinal();

  Serial.println();
  separador('=');
  Serial.println(" TESTES CONCLUIDOS");
  separador('=');
  Serial.println("Se todos passaram: circuito OK, pode medir corrente.");
  Serial.println("Se algum falhou: veja a mensagem de diagnostico.");
  Serial.println(); 
  Serial.println("Reinicie o ESP32 para rodar os testes novamente.");

  //conectarWiFi();
  //sincronizarRelogio();
}

// ============================================================
//  LOOP - Monitoramento limpo com RMS
// ============================================================
void loop() {
  // Coleta um pacote de amostras e calcula o RMS para estabilizar a leitura ao vivo
  AmostragemStats monitor = amostrarADC(PINO_ADC, NUM_AMOSTRAS_TESTE);

  // Converte RMS (Volts) para corrente (Amperes)
  float corrente = monitor.rmsTensao * AMPS_POR_VOLT;
  if (monitor.rmsTensao < LIMIAR_RUIDO) corrente = 0.0;  // descarta ruido de fundo

  // Linha compacta a cada amostra, so pra acompanhar ao vivo
  Serial.printf("Monitor RMS -> Offset: %.3fV | AC: %.4fV | Corrente: %.3fA\n",
                adcParaTensao(monitor.media), monitor.rmsTensao, corrente);

  somaCorrente += corrente;
  contadorAmostras++;

  // A cada AMOSTRAS_POR_ENVIO leituras, fecha a media e reporta
  if (contadorAmostras >= AMOSTRAS_POR_ENVIO) {
    float correnteMedia = somaCorrente / contadorAmostras;
    float potencia      = correnteMedia * TENSAO_REDE;   // potencia aparente (VA)

    // Mostra no Serial exatamente o que vai pro banco
    imprimirLeitura(correnteMedia, potencia);

    // if (enviarLeitura(correnteMedia, potencia)) {
    //   Serial.println(">>> Gravado na API");
    // } else {
    //   Serial.println(">>> FALHA no envio (leitura perdida)");
    // }

    somaCorrente = 0;
    contadorAmostras = 0;
  }

  delay(2000);
}

// ============================================================
//  FUNCOES AUXILIARES
// ============================================================


void imprimirLeitura(float correnteA, float potenciaVA) {
  String ts = timestampISO8601();

  // Sem NTP sincronizado, mostra tempo desde o boot como referencia
  bool relogioOk = (ts.length() > 0);
  if (!relogioOk) {
    ts = "T+" + String(millis() / 1000) + "s (sem NTP)";
  }
  
  Serial.println();
  separador('-');
  Serial.printf("Equipamento : %s\n",      EQUIPAMENTO);
  Serial.printf("Timestamp   : %s\n",      ts.c_str());
  Serial.printf("Corrente    : %.3f A\n",  correnteA);
  Serial.printf("Potencia    : %.1f VA\n", potenciaVA);
  separador('-');
  Serial.println();
}

// Le N amostras do pino aplicando o calculo RMS para isolar o sinal AC
AmostragemStats amostrarADC(int pino, int numAmostras, int atrasoMicros) {
  int   minimo = ADC_MAX;
  int   maximo = 0;
  double soma = 0;
  double somaQuadrados = 0;

  for (int i = 0; i < numAmostras; i++) {
    int leitura = analogRead(pino);
    if (leitura < minimo) minimo = leitura;
    if (leitura > maximo) maximo = leitura;
    
    soma += leitura;
    somaQuadrados += ((double)leitura * leitura); // Necessario para o calculo RMS
    
    delayMicroseconds(atrasoMicros);
  }

  double media = soma / numAmostras;
  double mediaQuadrados = somaQuadrados / numAmostras;

  // Formula da variancia: remove o Offset DC matematicamente
  double variancia = mediaQuadrados - (media * media);
  if (variancia < 0) variancia = 0;

  double rmsADC = sqrt(variancia);

  AmostragemStats stats;
  stats.minimo    = minimo;
  stats.maximo    = maximo;
  stats.variacao  = maximo - minimo;
  stats.media     = media;
  stats.rmsTensao = adcParaTensao(rmsADC); // Converte o RMS do ADC para Volts

  return stats;
}

// Converte uma leitura ADC (0-4095) para tensao em Volts
float adcParaTensao(float valorADC) {
  return (valorADC / (float)ADC_MAX) * VREF;
}

// Imprime uma linha separadora de 50 caracteres
void separador(char c) {
  for (int i = 0; i < 50; i++) Serial.print(c);
  Serial.println();
}

// ============================================================
//  TESTE 1 - OFFSET DC
// ============================================================
void teste1_offset() {
  separador('-');
  Serial.println(" TESTE 1: OFFSET DC");
  separador('-');
  Serial.println("Medindo o ponto medio do divisor de tensao...");
  Serial.println();

  AmostragemStats stats = amostrarADC(PINO_ADC, NUM_AMOSTRAS_TESTE);
  float diferenca = stats.media - OFFSET_ESPERADO;

  Serial.printf("Offset medido: %.1f (%.3fV)\n", stats.media, adcParaTensao(stats.media));
  Serial.printf("Offset esperado: %d (%.3fV)\n", OFFSET_ESPERADO, adcParaTensao(OFFSET_ESPERADO));
  Serial.printf("Diferenca: %.1f unidades ADC\n", diferenca);
  Serial.println();

  // Diagnostico
  if (abs(diferenca) <= OFFSET_TOLERANCIA) {
    Serial.println(">>> RESULTADO: PASSOU");
    Serial.println("    O divisor esta gerando ~1,65V como esperado.");
  } else if (stats.media > OFFSET_ESPERADO + OFFSET_TOLERANCIA) {
    Serial.println(">>> RESULTADO: FALHOU - Offset ALTO demais");
    Serial.println("    POSSIVEIS CAUSAS:");
    Serial.println("    1. Voce alimentou o divisor com 5V em vez de 3,3V");
    Serial.println("    2. R2 (resistor inferior) esta maior que R1");
    Serial.println("    3. Mau contato em R2");
    Serial.println("    ACAO: Verifique se o trilho + esta em 3,3V");
    Serial.println("          e meça R1 e R2 isoladamente com multimetro");
  } else {
    Serial.println(">>> RESULTADO: FALHOU - Offset BAIXO demais");
    Serial.println("    POSSIVEIS CAUSAS:");
    Serial.println("    1. R1 (resistor superior) esta maior que R2");
    Serial.println("    2. Mau contato em R1");
    Serial.println("    3. Capacitor invertido puxando o offset pra baixo");
    Serial.println("    ACAO: Meca cada resistor isoladamente e");
    Serial.println("          verifique polaridade do capacitor");
  }

  Serial.println();
  delay(1000);
}

// ============================================================
//  TESTE 2 - RUIDO
// ============================================================
void teste2_ruido() {
  separador('-');
  Serial.println(" TESTE 2: NIVEL DE RUIDO");
  separador('-');
  Serial.println("Medindo variacao das leituras...");
  Serial.println();

  AmostragemStats stats = amostrarADC(PINO_ADC, NUM_AMOSTRAS_TESTE);
  float variacaoTensaoMV = adcParaTensao(stats.variacao) * 1000;

  Serial.printf("Minimo lido: %d\n", stats.minimo);
  Serial.printf("Maximo lido: %d\n", stats.maximo);
  Serial.printf("Variacao (pico-a-pico): %d unidades (%.1f mV)\n", stats.variacao, variacaoTensaoMV);
  Serial.println();

  // Diagnostico
  if (stats.variacao <= RUIDO_MAXIMO_OK) {
    Serial.println(">>> RESULTADO: PASSOU");
    Serial.println("    Ruido esta em nivel aceitavel.");
    Serial.println("    O capacitor esta filtrando bem.");
  } else if (stats.variacao <= 200) {
    Serial.println(">>> RESULTADO: ALERTA - Ruido moderado");
    Serial.println("    O circuito funciona mas nao esta ideal.");
    Serial.println("    POSSIVEIS CAUSAS:");
    Serial.println("    1. Capacitor com valor menor que 10uF");
    Serial.println("    2. Interferencia eletromagnetica proxima");
    Serial.println("    3. Fonte USB do ESP32 muito ruidosa");
    Serial.println("    ACAO: teste em outro cabo/porta USB.");
  } else {
    Serial.println(">>> RESULTADO: FALHOU - Ruido excessivo");
    Serial.println("    O sinal esta muito instavel para medir.");
    Serial.println("    POSSIVEIS CAUSAS:");
    Serial.println("    1. Capacitor NAO conectado ou invertido");
    Serial.println("    2. Capacitor com defeito");
    Serial.println("    3. Fio muito longo entre divisor e ADC");
    Serial.println("    ACAO: verifique a polaridade do capacitor");
    Serial.println("          (perna longa = positivo, no ponto medio)");
  }

  Serial.println();
  delay(1000);
}

// ============================================================
//  TESTE 3 - DETECCAO DE SINAL AC
// ============================================================
void teste3_deteccaoSinal() {
  separador('-');
  Serial.println(" TESTE 3: DETECCAO DE SINAL DO SENSOR (COM RMS)");
  separador('-');
  Serial.println("Este teste mostra se o sensor esta captando algo.");
  Serial.println();
  Serial.println("VOCE VAI VER 10 MEDICOES:");
  Serial.println("  - Sem carga no fio pinçado: valor RMS baixo");
  Serial.println("  - Com carga: valor RMS aumenta proporcionalmente");
  Serial.println();
  Serial.println("Medicao | Variacao | Sinal AC (RMS)");
  Serial.println("--------+----------+----------------");

  for (int m = 0; m < 10; m++) {
    AmostragemStats stats = amostrarADC(PINO_ADC, NUM_AMOSTRAS_TESTE);
    float rmsMV = stats.rmsTensao * 1000; // Converte para mV para exibicao

    Serial.printf("   %d    |   %d    |   %.1f mV\n",
                  m + 1, stats.variacao, rmsMV);

    delay(500);
  }

  Serial.println();
  Serial.println(">>> COMO INTERPRETAR:");
  Serial.println("    - RMS < 30 mV: sem sinal significativo (ou sem corrente)");
  Serial.println("    - RMS 30-150 mV: corrente baixa");
  Serial.println("    - RMS > 150 mV: corrente moderada/alta");
  Serial.println();
  delay(1000);
}

// ------------------------------------------------------------
//  WIFI
// ------------------------------------------------------------
void conectarWiFi() {
  Serial.printf("Conectando em '%s'", WIFI_SSID_STR);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID_STR, WIFI_PASS_STR);

  // Reconecta sozinho se cair -- essencial pra rodar dias seguidos
  WiFi.setAutoReconnect(true);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("FALHOU. Segue medindo offline, tenta de novo no envio.");
  }
}

// ------------------------------------------------------------
//  NTP -- o ESP32 nao tem RTC com bateria
// ------------------------------------------------------------
void sincronizarRelogio() {
  // Grava sempre em UTC; a conversao pro fuso e' problema da API/dashboard
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizando relogio");
  struct tm t;
  unsigned long inicio = millis();
  while (!getLocalTime(&t, 1000) && millis() - inicio < 15000) {
    Serial.print(".");
  }
  Serial.println();

  if (getLocalTime(&t)) {
    Serial.printf("Relogio OK: %s", asctime(&t));
  } else {
    Serial.println("NTP falhou -- API vai usar o horario do servidor.");
  }
}

// Retorna ISO 8601 em UTC, ou string vazia se o relogio nao sincronizou
String timestampISO8601() {
  struct tm t;
  // timeout 0 = so consulta o relogio e retorna. Sem isso, o default de 5s
  // travaria o loop a cada chamada enquanto o NTP nao estiver sincronizado.
  if (!getLocalTime(&t, 0)) return "";

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(buffer);
}

// ------------------------------------------------------------
//  ENVIO HTTP
// ------------------------------------------------------------
bool enviarLeitura(float correnteA, float potenciaVA) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem WiFi -- tentando reconectar...");
    WiFi.reconnect();
    return false;
  }

  // ATENCAO: Current e Power sao string na API.
  // Mandar numero puro no JSON resulta em 400 Bad Request.
  char bufCorrente[16];
  char bufPotencia[16];
  dtostrf(correnteA,  0, 4, bufCorrente);
  dtostrf(potenciaVA, 0, 2, bufPotencia);

  JsonDocument doc;
  doc["equipment"] = EQUIPAMENTO;
  doc["current"]   = bufCorrente;
  doc["power"]     = bufPotencia;
  doc["location"]  = LOCALIZACAO;
  doc["unit"]      = UNIDADE;

  String ts = timestampISO8601();
  if (ts.length() > 0) doc["timestamp"] = ts;

  String payload;
  serializeJson(doc, payload);

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, API_URL)) {
    Serial.println("http.begin() falhou -- confira a URL");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);          // nao trava o loop se a API estiver fora
  http.setConnectTimeout(5000);

  int codigo = http.POST(payload);
  bool sucesso = (codigo >= 200 && codigo < 300);

  if (!sucesso) {
    Serial.printf("HTTP %d -- resposta: %s\n", codigo, http.getString().c_str());
  }

  http.end();
  return sucesso;
}