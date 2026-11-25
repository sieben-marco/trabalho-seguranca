/*
 * ============================================================================
 * Projeto BIBIFI IoT - Fase 1: Build It (Código inicial)
 * Baseado em: Fulton et al. (ACM CCS '22) - "Understanding the How and the Why"
 * ============================================================================
 * Estrutura (2 arquivos na mesma pasta):
 *   - 1_smarthome_esqueleto_colegas.ino : Wi-Fi, pinos, controle /luz e rotas
 *   - web.h                             : Interface HTML/CSS/JS em PROGMEM
 *
 * Hardware: NodeMCU ESP8266 (LoLin V3)
 *   - Pino D1 (GPIO 5) -> LED Azul (Luz Principal - inicia acesa)
 *   - Pino D2 (GPIO 4) -> LED Vermelho (Alarme de Segurança - 3 segundos)
 *
 * Rede Wi-Fi própria (SoftAP):
 *   - SSID:  SmartHome_IoT  | Senha: 12345678
 *   - Convidado: http://192.168.4.1/
 *   - Admin:     http://192.168.4.1/admin
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MD5Builder.h>
#include "web.h"

// --- Configuração da Rede Própria (Access Point) ---
const char* AP_SSID  = "SmartHome_IoT";
const char* AP_SENHA = "12345678";

// --- Credenciais do Administrador ---
const String ADMIN_USER  = "admin";
const String SENHA_ADMIN = "123";

// --- Mapeamento de Pinos da NodeMCU ---
const int PIN_LUZ_AZUL    = D1; // GPIO 5 - Luz Principal
const int PIN_ALARME_VERM = D2; // GPIO 4 - Alarme de Segurança
const unsigned long DURACAO_ALARME_MS = 3000;

ESP8266WebServer server(80);

// --- Estado do Sistema ---
bool luzAcesa = true;
unsigned long alarmeDesligaEm = 0;
String ultimoEventoLog = "Sistema iniciado. Luz principal acesa.";

// ============================================================================
// Funções auxiliares
// ============================================================================

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
  ultimoEventoLog = "[Alerta] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"status\":\"acesso_negado\",\"mensagem\":\"" + motivo + "\"}");
}

// ============================================================================
// Controlador da luz (rota /luz)
// Complete a lógica de autenticação dentro do bloco TODO abaixo.
// ============================================================================
void handleControleLuz() {
  String acao  = server.arg("acao");   // "ON" ou "OFF"
  String user  = server.arg("user");   // "admin" ou "visitante"
  String senha = server.arg("senha");  // Senha opcional
  String token = server.arg("token");  // Token Hash MD5 opcional

  // Qualquer perfil pode ligar a luz (ON)
  if (acao == "ON") {
    luzAcesa = true;
    digitalWrite(PIN_LUZ_AZUL, HIGH);
    ultimoEventoLog = "Luz acesa por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[Ok] " + ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"acesa\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  // Apenas o administrador autenticado pode apagar a luz (OFF)
  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      registrarAlertaSeguranca("Acesso negado: perfil [" + user + "] não pode apagar a luz.");
    } else {
      // ======================================================================
      // TODO: Implemente aqui a autenticação do administrador para o comando OFF.
      // O painel /admin envia o parâmetro `token` calculado por
      // gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF") (também pode aceitar `senha`).
      // Se a credencial for inválida, bloqueie o comando usando registrarAlertaSeguranca(...).
      // ======================================================================

    }

    luzAcesa = false;
    digitalWrite(PIN_LUZ_AZUL, LOW);
    ultimoEventoLog = "Luz apagada por [" + user + "]";
    Serial.println("[Ação] " + ultimoEventoLog);

    if (!server.client().connected()) return;
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"apagada\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  server.send(400, "application/json", "{\"status\":\"erro\",\"msg\":\"Use acao=ON ou acao=OFF\"}");
}

// ============================================================================
// Rotas de interface (PROGMEM) e API
// ============================================================================
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

void handleStatus() {
  String json = "{";
  json += "\"luz_acesa\":" + String(luzAcesa ? "true" : "false") + ",";
  json += "\"ultimo_evento\":\"" + ultimoEventoLog + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Endpoint auxiliar de diagnóstico de hardware
void handleManutencaoFabrica() {
  String cmd = server.arg("cmd");
  if (cmd == "apagar_tudo") {
    luzAcesa = false;
    digitalWrite(PIN_LUZ_AZUL, LOW);
    ultimoEventoLog = "[Manutenção] Luz apagada via /api/manutencao";
    Serial.println(ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Reset executado\"}");
    return;
  }
  server.send(400, "application/json", "{\"status\":\"erro\",\"msg\":\"Comando desconhecido\"}");
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

  Serial.println("\n----------------------------------------------------");
  Serial.println("Smart Home IoT iniciado (Fase Build It)");
  Serial.print("Rede Wi-Fi (SSID): "); Serial.println(AP_SSID);
  Serial.print("Convidado: http://"); Serial.println(WiFi.softAPIP());
  Serial.print("Admin:     http://"); Serial.print(WiFi.softAPIP()); Serial.println("/admin");
  Serial.println("----------------------------------------------------");
}

void loop() {
  server.handleClient();

  if (alarmeDesligaEm > 0 && (long)(millis() - alarmeDesligaEm) >= 0) {
    digitalWrite(PIN_ALARME_VERM, LOW);
    alarmeDesligaEm = 0;
  }
}
