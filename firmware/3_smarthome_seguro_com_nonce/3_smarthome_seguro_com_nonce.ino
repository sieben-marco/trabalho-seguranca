/*
 * ============================================================================
 * Projeto BIBIFI IoT - Fase 5: Solução definitiva anti-replay (com Nonce)
 * Baseado em: Fulton et al. (ACM CCS '22) - "Understanding the How and the Why"
 * ============================================================================
 * Estrutura (2 arquivos na mesma pasta):
 *   - 3_smarthome_seguro_com_nonce.ino : Controle de acesso + Nonce dinâmico
 *   - web.h                            : Interface HTML/CSS/JS em PROGMEM
 *
 * Hardware: NodeMCU ESP8266 — multiplexação dos LEDs para 1 resistor comum
 *   - Pino D1 (GPIO 5) -> LED Azul (Luz Principal - inicia acesa)
 *   - Pino D2 (GPIO 4) -> LED Vermelho (Alarme intermitente até login do admin)
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MD5Builder.h>
#include <Ticker.h>
#include "web.h"

const char* AP_SSID  = "SmartHome_IoT";
const char* AP_SENHA = "12345678";

const String ADMIN_USER  = "admin";
const String SENHA_ADMIN = "123";

const int PIN_LUZ_AZUL    = D1; // GPIO 5 - Luz Principal
const int PIN_ALARME_VERM = D2; // GPIO 4 - Alarme de Segurança

// Multiplexação dos LEDs (1 resistor compartilhado) e alarme intermitente
const unsigned long INTERVALO_PISCA_ALARME_MS = 700; // 700 ms aceso / 700 ms apagado
const unsigned long INTERVALO_MUX_MS          = 2;   // 2 ms por slot -> 4 slots = 8 ms (125 Hz)
const uint8_t       SLOTS_MUX                 = 4;   // 3 slots Azul (75%) + 1 slot Vermelho (25%)

ESP8266WebServer server(80);
Ticker muxTicker;

volatile bool luzAcesa = true;
volatile bool alarmeAtivo = false;
volatile unsigned long alarmeInicioMs = 0;
uint8_t slotMux = 0;
String nonceAtual = "";
String ultimoEventoLog = "Sistema blindado com Nonce anti-replay iniciado.";

void renovarNonce() {
  nonceAtual = String(random(100000, 999999)) + "_" + String(millis());
  Serial.println("[Nonce renovado] Novo desafio: " + nonceAtual);
}

String gerarTokenComNonce(String usuario, String senha, String acao, String nonce) {
  MD5Builder md5;
  md5.begin();
  md5.add(usuario + ":" + senha + ":" + acao + ":" + nonce);
  md5.calculate();
  return md5.toString();
}

void atualizarSaidaLeds() {
  bool querAzul = luzAcesa;
  bool querVermelho = alarmeAtivo &&
    (((millis() - alarmeInicioMs) / INTERVALO_PISCA_ALARME_MS) % 2 == 0);

  // Multiplexação (break-before-make): nunca deixa D1 e D2 em HIGH simultaneamente
  if (querAzul && querVermelho) {
    if (slotMux == 0) {
      digitalWrite(PIN_LUZ_AZUL, LOW);
      digitalWrite(PIN_ALARME_VERM, HIGH);
    } else {
      digitalWrite(PIN_ALARME_VERM, LOW);
      digitalWrite(PIN_LUZ_AZUL, HIGH);
    }
    slotMux = (slotMux + 1) % SLOTS_MUX;
  } else if (querAzul) {
    digitalWrite(PIN_ALARME_VERM, LOW);
    digitalWrite(PIN_LUZ_AZUL, HIGH);
    slotMux = 0;
  } else if (querVermelho) {
    digitalWrite(PIN_LUZ_AZUL, LOW);
    digitalWrite(PIN_ALARME_VERM, HIGH);
    slotMux = 0;
  } else {
    digitalWrite(PIN_LUZ_AZUL, LOW);
    digitalWrite(PIN_ALARME_VERM, LOW);
    slotMux = 0;
  }
}

void dispararLedAlarme() {
  if (!alarmeAtivo) {
    alarmeInicioMs = millis();
  }
  alarmeAtivo = true;
}

void desligarLedAlarme() {
  if (alarmeAtivo) {
    alarmeAtivo = false;
  }
}

void registrarAlertaSeguranca(String motivo) {
  dispararLedAlarme();
  ultimoEventoLog = "[Bloqueado] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(
    403,
    "application/json",
    "{\"status\":\"replay_ou_acesso_bloqueado\",\"mensagem\":\"" + motivo + "\",\"novo_nonce\":\"" + nonceAtual + "\"}"
  );
}

void handleControleLuz() {
  String acao  = server.arg("acao");
  String user  = server.arg("user");
  String token = server.arg("token");

  if (acao == "ON") {
    luzAcesa = true;
    ultimoEventoLog = "Luz acesa por [" + (user != "" ? user : "visitante") + "]";
    server.send(
      200,
      "application/json",
      "{\"status\":\"ok\",\"luz\":\"acesa\",\"msg\":\"" + ultimoEventoLog + "\",\"nonce_atual\":\"" + nonceAtual + "\"}"
    );
    return;
  }

  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      registrarAlertaSeguranca("Perfil [" + user + "] não autorizado.");
      return;
    }

    String tokenEsperado = gerarTokenComNonce(ADMIN_USER, SENHA_ADMIN, "OFF", nonceAtual);
    if (token == "" || token != tokenEsperado) {
      registrarAlertaSeguranca("Replay Attack detectado: token inválido ou Nonce já utilizado.");
      return;
    }

    luzAcesa = false;
    renovarNonce(); // Invalida o token que acabou de trafegar na rede!

    ultimoEventoLog = "Luz apagada com sucesso. Nonce anterior invalidado.";
    Serial.println("[Ok] " + ultimoEventoLog);
    server.send(
      200,
      "application/json",
      "{\"status\":\"ok\",\"luz\":\"apagada\",\"msg\":\"" + ultimoEventoLog + "\",\"novo_nonce\":\"" + nonceAtual + "\"}"
    );
    return;
  }

  server.send(400, "application/json", "{\"status\":\"erro\",\"msg\":\"Use acao=ON ou acao=OFF\"}");
}

void handleAdminLogin() {
  String u = server.arg("usuario");
  String s = server.arg("senha");
  if (u == ADMIN_USER && s == SENHA_ADMIN) {
    desligarLedAlarme();
    String tokenUnico = gerarTokenComNonce(ADMIN_USER, SENHA_ADMIN, "OFF", nonceAtual);
    server.send(
      200,
      "application/json",
      "{\"ok\":true,\"token\":\"" + tokenUnico + "\",\"nonce\":\"" + nonceAtual + "\"}"
    );
    return;
  }
  dispararLedAlarme();
  ultimoEventoLog = "[Alerta] Tentativa de login inválida em /admin";
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"ok\":false,\"mensagem\":\"Usuário ou senha inválidos.\"}");
}

void handleManutencaoFabrica() {
  registrarAlertaSeguranca("Rota /api/manutencao bloqueada.");
}

void handleStatus() {
  String json = "{";
  json += "\"luz_acesa\":" + String(luzAcesa ? "true" : "false") + ",";
  json += "\"alarme_ativo\":" + String(alarmeAtivo ? "true" : "false") + ",";
  json += "\"nonce_atual\":\"" + nonceAtual + "\",";
  json += "\"ultimo_evento\":\"" + ultimoEventoLog + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(analogRead(A0));

  pinMode(PIN_LUZ_AZUL, OUTPUT);
  pinMode(PIN_ALARME_VERM, OUTPUT);

  atualizarSaidaLeds();
  muxTicker.attach_ms(INTERVALO_MUX_MS, atualizarSaidaLeds);

  renovarNonce();

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_SENHA);

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", CONVIDADO_HTML);
  });
  server.on("/admin", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", ADMIN_HTML);
  });
  server.on("/admin", HTTP_POST, handleAdminLogin);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/luz", HTTP_GET, handleControleLuz);
  server.on("/api/manutencao", HTTP_GET, handleManutencaoFabrica);

  server.begin();
  Serial.println("\n[Anti-Replay] Smart Home iniciado em http://192.168.4.1");
}

void loop() {
  server.handleClient();
}
