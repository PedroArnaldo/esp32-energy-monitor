/*
 * ============================================================
 *  TESTE DE VALIDACAO DO CIRCUITO
 *  Projeto IC - Monitoramento Energetico
 * ============================================================
 *
 *  Este codigo NAO calcula corrente. Ele testa se o circuito
 *  de condicionamento (divisor + capacitor) esta funcionando
 *  corretamente ANTES de comecarmos a medir de verdade.
 *
 *  Vai executar 3 testes em sequencia:
 *    1. Offset DC (verifica se esta em ~1,65V)
 *    2. Ruido (verifica estabilidade)
 *    3. Detecao de sinal (verifica se responde ao sensor)
 */

#include <Arduino.h>

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
//  PROTOTIPOS
// ============================================================
void teste1_offset();
void teste2_ruido();
void teste3_deteccaoSinal();
float lerADCMedia(int amostras);
int lerADCMinimo(int amostras);
int lerADCMaximo(int amostras);

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
  Serial.println("==================================================");
  Serial.println(" TESTE DE VALIDACAO - Circuito de Offset SCT-013");
  Serial.println("==================================================");
  Serial.println();
  Serial.println("Aguardando 3 segundos para estabilizacao...");
  Serial.println("(o capacitor precisa carregar completamente)");
  delay(3000);

  // Executa os 3 testes em sequencia
  teste1_offset();
  teste2_ruido();
  teste3_deteccaoSinal();

  Serial.println();
  Serial.println("==================================================");
  Serial.println(" TESTES CONCLUIDOS");
  Serial.println("==================================================");
  Serial.println("Se todos passaram: circuito OK, pode medir corrente.");
  Serial.println("Se algum falhou: veja a mensagem de diagnostico.");
  Serial.println();
  Serial.println("Reinicie o ESP32 para rodar os testes novamente.");
}

// ============================================================
//  LOOP - so imprime status periodicamente
// ============================================================
void loop() {
  // Depois dos testes, so imprime leitura atual a cada 2s
  // pra voce poder monitorar o circuito ao vivo
  int leitura = analogRead(PINO_ADC);
  float tensao = (leitura / (float)ADC_MAX) * VREF;

  Serial.print("Monitor ao vivo -> ADC: ");
  Serial.print(leitura);
  Serial.print(" | Tensao: ");
  Serial.print(tensao, 3);
  Serial.println("V");

  delay(2000);
}

// ============================================================
//  TESTE 1 - OFFSET DC
// ============================================================
//  Pergunta: o ponto medio do divisor esta em ~1,65V?
//  Como: mede a MEDIA de 500 leituras (sem sensor plugado
//  ou com sensor sem corrente, a media = offset DC)
//
//  Aprovacao: offset entre 1798 e 2298 (equivale a
//  1,45V - 1,85V), com o alvo sendo 2048 (1,65V exato)
// ============================================================
void teste1_offset() {
  Serial.println("--------------------------------------------------");
  Serial.println(" TESTE 1: OFFSET DC");
  Serial.println("--------------------------------------------------");
  Serial.println("Medindo o ponto medio do divisor de tensao...");
  Serial.println();

  float media = lerADCMedia(NUM_AMOSTRAS_TESTE);
  float tensaoMedia = (media / ADC_MAX) * VREF;

  Serial.print("Offset medido: ");
  Serial.print(media, 1);
  Serial.print(" (");
  Serial.print(tensaoMedia, 3);
  Serial.println("V)");

  Serial.print("Offset esperado: ");
  Serial.print(OFFSET_ESPERADO);
  Serial.print(" (");
  Serial.print((OFFSET_ESPERADO / (float)ADC_MAX) * VREF, 3);
  Serial.println("V)");

  Serial.print("Diferenca: ");
  float diferenca = media - OFFSET_ESPERADO;
  Serial.print(diferenca, 1);
  Serial.println(" unidades ADC");

  Serial.println();

  // Diagnostico
  if (abs(diferenca) <= OFFSET_TOLERANCIA) {
    Serial.println(">>> RESULTADO: PASSOU");
    Serial.println("    O divisor esta gerando ~1,65V como esperado.");
  } else if (media > OFFSET_ESPERADO + OFFSET_TOLERANCIA) {
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
//  Pergunta: o sinal esta estavel (capacitor filtrando)?
//  Como: mede a VARIACAO (max - min) das leituras
//
//  Aprovacao: variacao <= 50 unidades ADC (~40mV)
//  Se muito ruidoso: capacitor com problema ou EMI forte
// ============================================================
void teste2_ruido() {
  Serial.println("--------------------------------------------------");
  Serial.println(" TESTE 2: NIVEL DE RUIDO");
  Serial.println("--------------------------------------------------");
  Serial.println("Medindo variacao das leituras...");
  Serial.println();

  int minimo = 4095;
  int maximo = 0;
  long soma = 0;

  for (int i = 0; i < NUM_AMOSTRAS_TESTE; i++) {
    int leitura = analogRead(PINO_ADC);
    if (leitura < minimo) minimo = leitura;
    if (leitura > maximo) maximo = leitura;
    soma += leitura;
    delayMicroseconds(200);
  }

  int variacao = maximo - minimo;
  float mediaLocal = (float)soma / NUM_AMOSTRAS_TESTE;
  float variacaoTensao = (variacao / (float)ADC_MAX) * VREF * 1000;  // em mV

  Serial.print("Minimo lido: ");
  Serial.println(minimo);
  Serial.print("Maximo lido: ");
  Serial.println(maximo);
  Serial.print("Variacao (pico-a-pico): ");
  Serial.print(variacao);
  Serial.print(" unidades (");
  Serial.print(variacaoTensao, 1);
  Serial.println(" mV)");

  Serial.println();

  // Diagnostico
  if (variacao <= RUIDO_MAXIMO_OK) {
    Serial.println(">>> RESULTADO: PASSOU");
    Serial.println("    Ruido esta em nivel aceitavel.");
    Serial.println("    O capacitor esta filtrando bem.");
  } else if (variacao <= 200) {
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
//  Pergunta: com sensor plugado, o circuito capta variacao?
//  Como: mede min e max ao longo de 500 amostras rapidas
//
//  SEM corrente no fio: variacao pequena (so ruido)
//  COM corrente no fio: variacao proporcional ao sinal AC
//
//  Este teste NAO julga se passou ou falhou - so mostra
//  o comportamento e voce interpreta.
// ============================================================
void teste3_deteccaoSinal() {
  Serial.println("--------------------------------------------------");
  Serial.println(" TESTE 3: DETECCAO DE SINAL DO SENSOR");
  Serial.println("--------------------------------------------------");
  Serial.println("Este teste mostra se o sensor esta captando algo.");
  Serial.println();
  Serial.println("VOCE VAI VER 10 MEDICOES:");
  Serial.println("  - Sem carga no fio pinçado: variacao pequena");
  Serial.println("  - Com carga: variacao aumenta proporcionalmente");
  Serial.println();
  Serial.println("Medicao | Min  | Max  | Variacao | Amplitude");
  Serial.println("--------+------+------+----------+----------");

  for (int m = 0; m < 10; m++) {
    int minimo = 4095;
    int maximo = 0;

    // Amostra rapido pra capturar o sinal AC de 60Hz
    for (int i = 0; i < NUM_AMOSTRAS_TESTE; i++) {
      int leitura = analogRead(PINO_ADC);
      if (leitura < minimo) minimo = leitura;
      if (leitura > maximo) maximo = leitura;
      delayMicroseconds(200);
    }

    int amplitude = maximo - minimo;
    float amplitudeMV = (amplitude / (float)ADC_MAX) * VREF * 1000;

    Serial.print("   ");
    Serial.print(m + 1);
    Serial.print("    | ");
    Serial.print(minimo);
    Serial.print(" | ");
    Serial.print(maximo);
    Serial.print(" |   ");
    Serial.print(amplitude);
    Serial.print("    | ");
    Serial.print(amplitudeMV, 0);
    Serial.println(" mV");

    delay(500);
  }

  Serial.println();
  Serial.println(">>> COMO INTERPRETAR:");
  Serial.println("    - Amplitude < 100 mV: sem sinal significativo");
  Serial.println("      (sensor sem corrente ou nada conectado)");
  Serial.println("    - Amplitude 100-500 mV: corrente baixa");
  Serial.println("    - Amplitude > 500 mV: corrente moderada/alta");
  Serial.println();
  Serial.println("    Se plugou o sensor e pincou um fio com corrente");
  Serial.println("    mas amplitude continua baixa, o sensor pode nao");
  Serial.println("    estar recebendo sinal (fio errado, mau contato).");
  Serial.println();
  delay(1000);
}

// ============================================================
//  FUNCOES AUXILIARES
// ============================================================

//  Le N amostras e retorna a media
float lerADCMedia(int amostras) {
  long soma = 0;
  for (int i = 0; i < amostras; i++) {
    soma += analogRead(PINO_ADC);
    delayMicroseconds(200);
  }
  return (float)soma / amostras;
}