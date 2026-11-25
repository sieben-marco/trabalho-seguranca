/*
 * ============================================================================
 * Projeto BIBIFI IoT - Fase 3: Pós-Fix (corrigido pelos colegas, vulnerável
 * ao Replay Attack / replicação de comando)
 * Baseado em: Fulton et al. (ACM CCS '22) - "Understanding the How and the Why"
 * ============================================================================
 * Estrutura (2 arquivos na mesma pasta):
 *   - 2_smarthome_pos_fix_colegas.ino : Controle de acesso + Token MD5 estático
 *   - web.h                           : Interface HTML/CSS/JS em PROGMEM
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MD5Builder.h>
#include "web.h"

const char* AP_SSID  = "SmartHome_IoT";
const char* AP_SENHA = "12345678";

const String ADMIN_USER  = "admin";
const String SENHA_ADMIN = "123";

const int PIN_LUZ_AZUL    = D1; // GPIO 5 - Luz Principal
const int PIN_ALARME_VERM = D2; // GPIO 4 - Alarme de Segurança
const unsigned long DURACAO_ALARME_MS = 3000;

ESP8266WebServer server(80);

bool luzAcesa = true;
unsigned long alarmeDesligaEm = 0;
String ultimoEventoLog = "Sistema Pós-Fix iniciado. Protegido por Token MD5.";

String gerarTokenMD5(String usuario, String senha, String acao) {
  MD5Builder md5;
  md5.begin();
  md5.add(usuario + ":" + senha + ":" + acao);
  md5.calculate();
  return md5.toString();
}

void dispararLedAlarme() {
  digitalWrite(PIN_ALARME_VERM, HIGH);
  alarmeDesligaEm = millis() + DURACAO_ALARME_MS;
}

void registrarAlertaSeguranca(String motivo) {
  dispararLedAlarme();
  ultimoEventoLog = "[Bloqueado] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"status\":\"acesso_negado\",\"mensagem\":\"" + motivo + "\"}");
}

void handleControleLuz() {
  String acao  = server.arg("acao");
  String user  = server.arg("user");
  String token = server.arg("token");

  if (acao == "ON") {
    luzAcesa = true;
    digitalWrite(PIN_LUZ_AZUL, HIGH);
    ultimoEventoLog = "Luz acesa por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[Ok] " + ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"acesa\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      registrarAlertaSeguranca("Perfil [" + user + "] não tem permissão para apagar a luz.");
      return;
    }

    String tokenEsperado = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    if (token == "" || token != tokenEsperado) {
      registrarAlertaSeguranca("Token MD5 ausente ou inválido para o comando OFF.");
      return;
    }

    // Vulnerável ao Replay Attack: o token MD5 é estático!
    luzAcesa = false;
    digitalWrite(PIN_LUZ_AZUL, LOW);
    ultimoEventoLog = "Luz apagada com Token MD5 válido.";
    Serial.println("[Ação] " + ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"apagada\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  server.send(400, "application/json", "{\"status\":\"erro\",\"msg\":\"Use acao=ON ou acao=OFF\"}");
}

void handleAdminLogin() {
  String u = server.arg("usuario");
  String s = server.arg("senha");
  if (u == ADMIN_USER && s == SENHA_ADMIN) {
    String token = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    server.send(200, "application/json", "{\"ok\":true,\"token\":\"" + token + "\"}");
    return;
  }
  dispararLedAlarme();
  ultimoEventoLog = "[Alerta] Tentativa de login inválida em /admin";
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"ok\":false,\"mensagem\":\"Usuário ou senha inválidos.\"}");
}

void handleManutencaoFabrica() {
  registrarAlertaSeguranca("Rota /api/manutencao desativada no Fix It.");
}

void handleStatus() {
  String json = "{";
  json += "\"luz_acesa\":" + String(luzAcesa ? "true" : "false") + ",";
  json += "\"ultimo_evento\":\"" + ultimoEventoLog + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_LUZ_AZUL, OUTPUT);
  pinMode(PIN_ALARME_VERM, OUTPUT);

  digitalWrite(PIN_LUZ_AZUL, HIGH);
  digitalWrite(PIN_ALARME_VERM, LOW);

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
  Serial.println("\n[Pós-Fix] Smart Home iniciado em http://192.168.4.1");
}

void loop() {
  server.handleClient();

  if (alarmeDesligaEm > 0 && (long)(millis() - alarmeDesligaEm) >= 0) {
    digitalWrite(PIN_ALARME_VERM, LOW);
    alarmeDesligaEm = 0;
  }
}
