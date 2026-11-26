/*
 * ============================================================================
 * Etapa 4 — XIAO #2 (Hub Web): AES-128 + Nonce Aleatório (Desafio-Resposta)
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 * Estrutura (2 arquivos na mesma pasta):
 *   - etapa4_hub_web_nonce.ino : SoftAP Wi-Fi (Canal 1) + AES-128 + Nonce
 *   - web.h                    : Interface HTML/CSS/JS em PROGMEM (intacto)
 *
 * Evolução da Etapa 3 -> 4:
 *   - Antes de cada comando, solicita um desafio (Nonce de 32 bits gerado por
 *     hardware na Casa #1 via `esp_random()`), inclui esse Nonce dentro do
 *     bloco AES-128 e envia. Imune a Replay e a reinicialização da Casa (#1)!
 * ============================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <MD5Builder.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <mbedtls/aes.h>
#include <string.h>
#include "web.h"

// ====================== CONFIGURAÇÃO (PREENCHA O MAC) ======================
static const uint8_t CANAL_WIFI = 1;
static const char* AP_SSID      = "SmartHome_IoT";
static const char* AP_SENHA     = "12345678";

// Cole aqui o MAC STA da XIAO #1 (Casa), descoberto com 0_mostra_mac.ino:
static uint8_t CASA_MAC[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01 };

// Chave AES-128 (16 bytes) — deve ser IDÊNTICA na XIAO #1 (Casa)
static const uint8_t CHAVE_AES[16] = {
  'S','m','a','r','t','H','o','m','e','2','0','2','6','K','e','y'
};
static const uint8_t MAGIC_TAG[4] = { 'S', 'H', '2', '6' };
// ===========================================================================

static const String ADMIN_USER  = "admin";
static const String SENHA_ADMIN = "123";

static const uint8_t CMD_ON        = 'L';
static const uint8_t CMD_OFF       = 'D';
static const uint8_t CMD_ALARME    = 'A';
static const uint8_t CMD_SILENCIAR = 'S';
static const uint8_t CMD_REQ_NONCE = '?';

static const unsigned long NONCE_WAIT_TIMEOUT_MS = 500;

struct __attribute__((packed)) NonceResp {
  uint8_t tipo;   // 'N' (0x4E)
  uint32_t nonce;
};

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

static const uint8_t RX_Q_N = 12;
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static RxPkt rxQueue[RX_Q_N];

static WebServer server(80);
static bool luzAcesa = true;
static bool alarmeAtivo = false;
static bool nonceRecebidoFlag = false;
static uint32_t nonceRecebidoValor = 0;
static String ultimoEventoLog = "Sistema iniciado (Etapa 4 - AES + Nonce).";

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

static void preencherAleatorio(uint8_t* dst, size_t n) {
  for (size_t i = 0; i < n; i += 4) {
    uint32_t r = esp_random();
    for (size_t j = 0; j < 4 && (i + j) < n; j++) {
      dst[i + j] = (uint8_t)((r >> (8 * j)) & 0xFF);
    }
  }
}

static void aesCifrar16(const uint8_t in[16], uint8_t out[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, CHAVE_AES, 128);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
  mbedtls_aes_free(&ctx);
}

static String gerarTokenMD5(const String& usuario, const String& senha, const String& acao) {
  MD5Builder md5;
  md5.begin();
  md5.add(usuario + ":" + senha + ":" + acao);
  md5.calculate();
  return md5.toString();
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

    if (pkt.len == sizeof(NonceResp) && pkt.data[0] == 'N') {
      NonceResp resp;
      memcpy(&resp, pkt.data, sizeof(NonceResp));
      nonceRecebidoValor = resp.nonce;
      nonceRecebidoFlag = true;
    } else if (pkt.len == sizeof(StatusMsg) && pkt.data[0] == 'T') {
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

static bool solicitarNonce(uint32_t& nonceOut) {
  for (int tentativa = 0; tentativa < 2; tentativa++) {
    nonceRecebidoFlag = false;
    uint8_t req = CMD_REQ_NONCE;
    esp_now_send(CASA_MAC, &req, 1);

    unsigned long inicio = millis();
    while ((millis() - inicio) < NONCE_WAIT_TIMEOUT_MS) {
      drenarFilaEspNow();
      if (nonceRecebidoFlag) {
        nonceOut = nonceRecebidoValor;
        nonceRecebidoFlag = false;
        return true;
      }
      delay(2);
    }
  }
  Serial.println("[Erro] Timeout aguardando desafio Nonce da Casa (#1).");
  return false;
}

static bool enviarComandoEspNow(uint8_t cmd) {
  uint32_t nonce = 0;
  if (!solicitarNonce(nonce)) {
    return false;
  }

  // Layout Etapa 4 (16 bytes): [0]=cmd | [1..4]=nonce | [5..8]="SH26" | [9..15]=aleatório
  uint8_t claro[16];
  uint8_t cifrado[16];
  claro[0] = cmd;
  memcpy(claro + 1, &nonce, sizeof(nonce));
  memcpy(claro + 5, MAGIC_TAG, 4);
  preencherAleatorio(claro + 9, 7);

  aesCifrar16(claro, cifrado);
  esp_now_send(CASA_MAC, cifrado, 16);
  Serial.printf("[Etapa 4 - Hub] Enviado cmd='%c' com Nonce=0x%08lX (AES-128).\n",
                (char)cmd, (unsigned long)nonce);
  return true;
}

static void registrarAlertaSeguranca(const String& motivo) {
  alarmeAtivo = true;
  enviarComandoEspNow(CMD_ALARME);
  ultimoEventoLog = "[Bloqueado] " + motivo;
  Serial.println(ultimoEventoLog);
  server.send(403, "application/json", "{\"status\":\"acesso_negado\",\"mensagem\":\"" + motivo + "\"}");
}

static void handleControleLuz() {
  String acao  = server.arg("acao");
  String user  = server.arg("user");
  String token = server.arg("token");

  if (acao == "ON") {
    if (!enviarComandoEspNow(CMD_ON)) {
      server.send(504, "application/json", "{\"status\":\"erro\",\"msg\":\"Casa não respondeu ao desafio Nonce\"}");
      return;
    }
    luzAcesa = true;
    ultimoEventoLog = "Luz acesa por [" + (user != "" ? user : "visitante") + "]";
    Serial.println("[Ok] " + ultimoEventoLog);
    server.send(200, "application/json", "{\"status\":\"ok\",\"luz\":\"acesa\",\"msg\":\"" + ultimoEventoLog + "\"}");
    return;
  }

  if (acao == "OFF") {
    if (user != ADMIN_USER) {
      registrarAlertaSeguranca("Acesso negado: perfil [" + user + "] não pode apagar a luz.");
      return;
    }

    String tokenEsperado = gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF");
    if (token == "" || token != tokenEsperado) {
      registrarAlertaSeguranca("Token inválido para o comando OFF.");
      return;
    }

    if (!enviarComandoEspNow(CMD_OFF)) {
      server.send(504, "application/json", "{\"status\":\"erro\",\"msg\":\"Casa não respondeu ao desafio Nonce\"}");
      return;
    }
    luzAcesa = false;
    ultimoEventoLog = "Luz apagada (AES-128 + Nonce).";
    Serial.println("[Ação] " + ultimoEventoLog);
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
    memcpy(peer.peer_addr, CASA_MAC, 6);
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
  Serial.println("[Etapa 4 - Hub Web] Iniciado (ESP-NOW Unicast AES-128 + Nonce)");
  Serial.print("Wi-Fi SSID: "); Serial.println(AP_SSID);
  Serial.print("Painel Web: http://"); Serial.println(WiFi.softAPIP());
  Serial.println("----------------------------------------------------");
}

void loop() {
  drenarFilaEspNow();
  server.handleClient();
}
