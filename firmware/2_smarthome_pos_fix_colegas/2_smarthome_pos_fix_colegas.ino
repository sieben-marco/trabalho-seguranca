/*
 * ============================================================================
 * PROJETO BIBIFI IoT - FASE 3: PÓS-FIX (CORRIGIDO PELOS COLEGAS, MAS VULNERÁVEL
 * AO REPLAY ATTACK / REPLICAÇÃO DE COMANDO)
 * Baseado em: Fulton et al. (ACM CCS '22) - "Understanding the How and the Why"
 * ============================================================================
 * Melhorias incluídas:
 *   - LED Vermelho apaga automaticamente após 3 segundos.
 *   - Páginas separadas: Convidado (/) e Administrador com Login (/admin).
 *   - Correção do `return;` no RBAC e bloqueio da rota `/api/manutencao`.
 *   - Autenticação do comando OFF via Token Hash MD5 estático (sem Nonce),
 *     permanecendo vulnerável ao Replay Attack!
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MD5Builder.h>

const char* AP_SSID  = "SmartHome_IoT";
const char* AP_SENHA = "12345678";

const String ADMIN_USER  = "admin";
const String SENHA_ADMIN = "seguranca2026";

const int PIN_LUZ_AZUL    = D1; // GPIO 5 - Luz Principal da Sala
const int PIN_ALARME_VERM = D2; // GPIO 4 - Alarme de Segurança
const unsigned long DURACAO_ALARME_MS = 3000;

ESP8266WebServer server(80);

bool luzAcesa = true;
unsigned long alarmeDesligaEm = 0;
String ultimoEventoLog = "Sistema Pós-Fix iniciado. Protegido por Token MD5.";
String ultimoPacoteRede = "";

String gerarTokenMD5(String usuario, String senha, String acao) {
  MD5Builder md5;
  md5.begin();
  md5.add(usuario + ":" + senha + ":" + acao);
  md5.calculate();
  return md5.toString();
}

void registrarAlertaSeguranca(String motivo) {
  digitalWrite(PIN_ALARME_VERM, HIGH);
  alarmeDesligaEm = millis() + DURACAO_ALARME_MS;
  ultimoEventoLog = "[BLOQUEADO PELA DEFESA] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json",
              "{\"status\":\"ACESSO_NEGADO\",\"mensagem\":\"" + motivo + "\"}");
}

void handleControleLuz() {
  String acao  = server.arg("acao");
  String user  = server.arg("user");
  String token = server.arg("token");

  ultimoPacoteRede = "/luz?acao=" + acao + "&user=" + user +
                     (token != "" ? ("&token=" + token) : "");

  if (acao == "ON") {
    luzAcesa = true;
    digitalWrite(PIN_LUZ_AZUL, HIGH);
    ultimoEventoLog = "Luz ACESA por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[OK] " + ultimoEventoLog);
    server.send(200, "application/json",
                "{\"status\":\"OK\",\"luz\":\"ACESA\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      registrarAlertaSeguranca("Perfil [" + user + "] nao tem permissao para apagar a luz!");
      return;
    }

    String tokenEsperado = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    if (token == "" || token != tokenEsperado) {
      registrarAlertaSeguranca("Token MD5 ausente ou invalido para o comando OFF!");
      return;
    }

    // Vulnerável ao Replay Attack: o token MD5 é estático!
    luzAcesa = false;
    digitalWrite(PIN_LUZ_AZUL, LOW);
    ultimoEventoLog = "Luz APAGADA com Token MD5 valido!";
    Serial.println("[ACAO CRITICA] " + ultimoEventoLog);
    server.send(200, "application/json",
                "{\"status\":\"OK\",\"luz\":\"APAGADA\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  server.send(400, "application/json", "{\"status\":\"ERRO\",\"msg\":\"Use acao=ON ou acao=OFF\"}");
}

void handleManutencaoFabrica() {
  registrarAlertaSeguranca("Rota /api/manutencao foi desativada pela equipe no Fix It!");
}

void handleStatus() {
  String json = "{";
  json += "\"luz_acesa\":" + String(luzAcesa ? "true" : "false") + ",";
  json += "\"ultimo_evento\":\"" + ultimoEventoLog + "\",";
  json += "\"ultimo_pacote_rede\":\"" + ultimoPacoteRede + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handlePaginaConvidado() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Home (Pós-Fix) - Convidado</title>";
  html += "<style>body{font-family:sans-serif;background:#121826;color:#f3f4f6;max-width:560px;margin:20px auto;padding:15px}";
  html += ".card{background:#1f2937;padding:16px;border-radius:10px;margin-bottom:15px;border:1px solid #374151}";
  html += ".badge{display:inline-block;padding:6px 12px;border-radius:6px;font-weight:bold}";
  html += ".on{background:#2563eb;color:#fff}.off{background:#4b5563;color:#d1d5db}";
  html += "a.btn{display:inline-block;padding:10px 14px;margin:5px 6px 5px 0;border-radius:6px;text-decoration:none;color:#fff;font-weight:bold}";
  html += ".btn-blue{background:#3b82f6}.btn-red{background:#ef4444}.btn-gray{background:#6b7280}.btn-amber{background:#d97706}</style></head><body>";

  html += "<h2>🛡️ Smart Home IoT (Pós-Fix) — Convidado</h2>";

  html += "<div class='card'><h3>Estado da Luz Principal</h3>";
  html += "<span class='badge " + String(luzAcesa ? "on'>💡 LUZ ACESA" : "off'>🌑 LUZ APAGADA") + "</span>";
  html += "<p><b>Último evento:</b> " + ultimoEventoLog + "</p></div>";

  html += "<div class='card'><h3>Controles de Visitante</h3>";
  html += "<a class='btn btn-blue' href='/luz?acao=ON&user=visitante'>💡 Ligar Luz</a>";
  html += "<a class='btn btn-red' href='/luz?acao=OFF&user=visitante'>🌑 Tentar Apagar Luz (403)</a></div>";

  html += "<a class='btn btn-amber' href='/admin'>🔐 Acesso Administrador</a> ";
  html += "<a class='btn btn-gray' href='/'>🔄 Atualizar</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handlePainelAdmin() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Home (Pós-Fix) - Admin</title>";
  html += "<style>body{font-family:sans-serif;background:#121826;color:#f3f4f6;max-width:560px;margin:20px auto;padding:15px}";
  html += ".card{background:#1f2937;padding:16px;border-radius:10px;margin-bottom:15px;border:1px solid #374151}";
  html += "input{width:100%;box-sizing:border-box;padding:10px;margin:6px 0 12px;border-radius:6px;border:1px solid #4b5563;background:#111827;color:#fff}";
  html += "button,a.btn{display:inline-block;padding:10px 14px;margin:5px 6px 5px 0;border-radius:6px;text-decoration:none;color:#fff;font-weight:bold;border:none;cursor:pointer}";
  html += ".btn-blue{background:#3b82f6}.btn-red{background:#ef4444}.btn-gray{background:#6b7280}.erro{color:#f87171;font-weight:bold}</style></head><body>";

  html += "<h2>🔐 Smart Home IoT — Administrador</h2>";

  bool autenticado = false;
  String erroLogin = "";

  if (server.method() == HTTP_POST) {
    String u = server.arg("usuario");
    String s = server.arg("senha");
    if (u == ADMIN_USER && s == SENHA_ADMIN) {
      autenticado = true;
    } else {
      digitalWrite(PIN_ALARME_VERM, HIGH);
      alarmeDesligaEm = millis() + DURACAO_ALARME_MS;
      ultimoEventoLog = "[ALERTA] Login invalido em /admin!";
      Serial.println(ultimoEventoLog);
      erroLogin = "Usuário ou senha inválidos!";
    }
  }

  if (autenticado) {
    String tokenAdminOff = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    html += "<div class='card'><h3>Painel Autenticado (Admin)</h3>";
    html += "<a class='btn btn-blue' href='/luz?acao=ON&user=admin'>💡 Ligar Luz</a>";
    html += "<a class='btn btn-red' href='/luz?acao=OFF&user=admin&token=" + tokenAdminOff + "'>🌑 Apagar Luz (Token MD5)</a></div>";
  } else {
    html += "<div class='card'><h3>Login do Administrador</h3>";
    if (erroLogin != "") {
      html += "<p class='erro'>" + erroLogin + "</p>";
    }
    html += "<form method='POST' action='/admin'>";
    html += "<label>Usuário</label><input type='text' name='usuario' placeholder='admin' required>";
    html += "<label>Senha</label><input type='password' name='senha' placeholder='Digite a senha' required>";
    html += "<button type='submit' class='btn-blue'>Entrar</button>";
    html += "</form></div>";
  }

  html += "<a class='btn btn-gray' href='/'>⬅ Voltar para Área do Convidado</a>";
  html += "</body></html>";
  server.send(autenticado || server.method() == HTTP_GET ? 200 : 403, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_LUZ_AZUL, OUTPUT);
  pinMode(PIN_ALARME_VERM, OUTPUT);

  digitalWrite(PIN_LUZ_AZUL, HIGH);
  digitalWrite(PIN_ALARME_VERM, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_SENHA);

  server.on("/", handlePaginaConvidado);
  server.on("/admin", handlePainelAdmin);
  server.on("/status", handleStatus);
  server.on("/luz", handleControleLuz);
  server.on("/api/manutencao", handleManutencaoFabrica);

  server.begin();
  Serial.println("\n[POS-FIX] Smart Home iniciado em http://192.168.4.1");
}

void loop() {
  server.handleClient();

  if (alarmeDesligaEm > 0 && (long)(millis() - alarmeDesligaEm) >= 0) {
    digitalWrite(PIN_ALARME_VERM, LOW);
    alarmeDesligaEm = 0;
  }
}
