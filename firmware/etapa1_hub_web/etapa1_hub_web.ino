/*
 * ============================================================================
 * Etapa 1 — XIAO #2 (Hub Web): Falha de Fluxo (return faltando) + Texto Claro
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 * Estrutura (2 arquivos na mesma pasta):
 *   - etapa1_hub_web.ino : SoftAP Wi-Fi (Canal 1), rotas HTTP e ponte ESP-NOW
 *   - web.h              : Interface HTML/CSS/JS em PROGMEM (design intacto)
 *
 * Vulnerabilidades demonstradas nesta etapa:
 *   1. Mistake (Equipe T6 do artigo CCS '22): falta `return;` após negar OFF
 *      para visitante em handleControleLuz() -> dispara alarme E apaga a luz!
 *   2. ESP-NOW em Broadcast e texto claro (1 byte): Sniffer lê tudo no ar.
 * ============================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <MD5Builder.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <string.h>
#include "web.h"

// --- Rede Wi-Fi SoftAP (mesmo canal do ESP-NOW!) ---
static const uint8_t CANAL_WIFI = 1;
static const char* AP_SSID      = "SmartHome_IoT";
static const char* AP_SENHA     = "12345678";

// --- Credenciais do Administrador ---
static const String ADMIN_USER  = "admin";
static const String SENHA_ADMIN = "123";

static const uint8_t BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// --- Comandos ESP-NOW (Etapa 1: 1 byte em texto claro) ---
static const uint8_t CMD_ON        = 'L';
static const uint8_t CMD_OFF       = 'D';
static const uint8_t CMD_ALARME    = 'A';
static const uint8_t CMD_SILENCIAR = 'S';

struct __attribute__((packed)) StatusMsg {
  uint8_t tipo;        // 'T' (0x54)
  uint8_t luzAcesa;    // 0 ou 1
  uint8_t alarmeAtivo; // 0 ou 1
  char evento[48];
};

struct RxPkt {
  uint8_t data[64];
  uint8_t len;
};

static const uint8_t RX_Q_N = 8;
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static RxPkt rxQueue[RX_Q_N];

static WebServer server(80);
static bool luzAcesa = true;
static bool alarmeAtivo = false;
static String ultimoEventoLog = "Sistema iniciado (Etapa 1 - texto claro).";

static void configurarAntenaInterna() {
#ifdef WIFI_ENABLE
  pinMode(WIFI_ENABLE, OUTPUT);
  digitalWrite(WIFI_ENABLE, LOW);
  delay(10);
#endif
#ifdef WIFI_ANT_CONFIG
  pinMode(WIFI_ANT_CONFIG, OUTPUT);
  digitalWrite(WIFI_ANT_CONFIG, LOW);
#endif
}

static String gerarTokenMD5(const String& usuario, const String& senha, const String& acao) {
  MD5Builder md5;
  md5.begin();
  md5.add(usuario + ":" + senha + ":" + acao);
  md5.calculate();
  return md5.toString();
}

static void enviarComandoEspNow(uint8_t cmd) {
  esp_now_send(BROADCAST, &cmd, 1);
}

static void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (!data || len <= 0) return;
  uint8_t next = (rxHead + 1) % RX_Q_N;
  if (next == rxTail) return;
  uint8_t copyLen = len > (int)sizeof(rxQueue[rxHead].data) ? (uint8_t)sizeof(rxQueue[rxHead].data) : (uint8_t)len;
  memcpy(rxQueue[rxHead].data, data, copyLen);
  rxQueue[rxHead].len = copyLen;
  rxHead = next;
}

static void drenarFilaEspNow() {
  while (rxTail != rxHead) {
    RxPkt pkt = rxQueue[rxTail];
    rxTail = (rxTail + 1) % RX_Q_N;
    if (pkt.len == sizeof(StatusMsg) && pkt.data[0] == 'T') {
      StatusMsg st;
      memcpy(&st, pkt.data, sizeof(StatusMsg));
      st.evento[sizeof(st.evento) - 1] = '\0';
      luzAcesa = (st.luzAcesa != 0);
      alarmeAtivo = (st.alarmeAtivo != 0);
      if (st.evento[0] != '\0') {
        ultimoEventoLog = String(st.evento);
      }
    }
  }
}

static void registrarAlertaSeguranca(const String& motivo) {
  alarmeAtivo = true;
  enviarComandoEspNow(CMD_ALARME);
  ultimoEventoLog = "[Alerta] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"status\":\"acesso_negado\",\"mensagem\":\"" + motivo + "\"}");
  server.client().stop();
}

// ============================================================================
// Controlador da luz (rota /luz) — Etapa 1 com bug de fluxo (falta return;)
// ============================================================================
static void handleControleLuz() {
  String acao  = server.arg("acao");
  String user  = server.arg("user");
  String token = server.arg("token");

  if (acao == "ON") {
    enviarComandoEspNow(CMD_ON);
    luzAcesa = true;
    ultimoEventoLog = "Luz acesa por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[Ok] " + ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"acesa\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      // ERRO PROPOSITAL (Mistake — Equipe T6 do artigo CCS '22):
      // dispara o alerta e devolve HTTP 403, mas esquece o `return;`!
      registrarAlertaSeguranca("Acesso negado: perfil [" + user + "] não pode apagar a luz.");
    } else {
      String tokenEsperado = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
      if (token == "" || token != tokenEsperado) {
        registrarAlertaSeguranca("Token inválido para o comando OFF.");
        return;
      }
    }

    delay(20);
    enviarComandoEspNow(CMD_OFF);
    luzAcesa = false;
    ultimoEventoLog = "Luz apagada por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[Ação] " + ultimoEventoLog);

    if (!server.client().connected()) return;
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"apagada\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  server.send(400, "application/json", "{\"status\":\"erro\",\"msg\":\"Use acao=ON ou acao=OFF\"}");
}

static void handleAdminLogin() {
  String u = server.arg("usuario");
  String s = server.arg("senha");
  if (u == ADMIN_USER && s == SENHA_ADMIN) {
    if (alarmeAtivo) {
      alarmeAtivo = false;
      ultimoEventoLog = "Alarme silenciado após login do administrador.";
    }
    enviarComandoEspNow(CMD_SILENCIAR);
    String token = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    server.send(200, "application/json", "{\"ok\":true,\"token\":\"" + token + "\"}");
    return;
  }
  alarmeAtivo = true;
  enviarComandoEspNow(CMD_ALARME);
  ultimoEventoLog = "[Alerta] Tentativa de login inválida em /admin";
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"ok\":false,\"mensagem\":\"Usuário ou senha inválidos.\"}");
}

static void handleStatus() {
  String json = "{";
  json += "\"luz_acesa\":" + String(luzAcesa ? "true" : "false") + ",";
  json += "\"alarme_ativo\":" + String(alarmeAtivo ? "true" : "false") + ",";
  json += "\"ultimo_evento\":\"" + ultimoEventoLog + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(250);

  configurarAntenaInterna();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_SENHA, CANAL_WIFI);
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST, 6);
    peer.channel = 0;
    peer.ifidx   = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
    esp_now_register_recv_cb(onEspNowRecv);
  }

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", CONVIDADO_HTML);
  });
  server.on("/admin", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", ADMIN_HTML);
  });
  server.on("/admin", HTTP_POST, handleAdminLogin);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/luz", HTTP_GET, handleControleLuz);

  server.begin();

  Serial.println("\n----------------------------------------------------");
  Serial.println("[Etapa 1 - Hub Web] Iniciado (SoftAP Canal 1 + ESP-NOW Broadcast)");
  Serial.print("Wi-Fi SSID: "); Serial.println(AP_SSID);
  Serial.print("Painel Web: http://"); Serial.println(WiFi.softAPIP());
  Serial.println("----------------------------------------------------");
}

void loop() {
  drenarFilaEspNow();
  server.handleClient();
}
