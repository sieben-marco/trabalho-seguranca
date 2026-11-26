/*
 * ============================================================================
 * XIAO #3 — Sniffer ESP-NOW em Modo Monitor + Replay Verbatim
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 * Monitor Serial: 115200 baud (sem necessidade de pinos soldados — apenas USB-C)
 *
 * Grave UMA ÚNICA VEZ na XIAO #3. Funciona em todas as 4 etapas sem regravar:
 *   - Etapa 1 (texto claro): mostra o comando legível (LIGAR / APAGAR / ALARME / SILENCIAR)
 *   - Etapas 2, 3 e 4 (AES-128): exibe "? (cifrado AES)" e repete os bytes exatos (verbatim)
 *
 * Comandos no Monitor Serial (digite e tecle Enter):
 *   l        lista o histórico numerado
 *   <n>      replay verbatim do pacote #n (ex.: 1)
 *   <n> xK   replay do pacote #n repetido K vezes (ex.: 1 x3)
 *   u        replay do último pacote capturado
 *   a        alterna exibição de pacotes de retorno (STATUS / RESP_NONCE)
 *   m        liga/desliga impressão ao vivo
 *   c        limpa o histórico
 *   v        varre os canais 1-13 contando pacotes ESP-NOW
 *   k <n>    muda para o canal n (salvo em flash)
 *   h        ajuda
 * Botão BOOT (GPIO 9): replay rápido do último pacote capturado.
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Preferences.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t CANAL_PADRAO = 1;
static const int     VARRE_MS     = 600;

#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif
#define PINO_LED        LED_BUILTIN  // XIAO ESP32-C6: GPIO 15 (ativo em LOW)
#define LED_ATIVO_BAIXO 1
#define PINO_BOTAO      9            // Botão BOOT da XIAO ESP32-C6
#define BODY_MAX        64
#define HIST_N          40
#define QN              16

static const uint8_t BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

struct Cap {
  uint8_t  src[6];
  uint8_t  dst[6];
  uint8_t  body[BODY_MAX];
  uint8_t  len;
  bool     bcast;
  int8_t   rssi;
  uint32_t ms;
  long     id;
};

static volatile uint8_t q_head = 0;
static volatile uint8_t q_tail = 0;
static Cap q[QN];

static Cap  hist[HIST_N];
static long total = 0;
static long base  = 1;
static bool ao_vivo = true;
static bool mostrar_todos = false;
static uint8_t canal = CANAL_PADRAO;
static uint8_t meu_mac[6] = {0};
static volatile bool varrendo = false;
static volatile uint32_t conta_varre = 0;
static Preferences prefs;

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

static void ledSet(bool on) {
#if LED_ATIVO_BAIXO
  digitalWrite(PINO_LED, on ? LOW : HIGH);
#else
  digitalWrite(PINO_LED, on ? HIGH : LOW);
#endif
}

static void ledPisca(int vezes, int ms) {
  for (int i = 0; i < vezes; i++) {
    ledSet(true);
    delay(ms);
    ledSet(false);
    delay(ms);
  }
}

static void macStr(const uint8_t* m, char* out) {
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static bool ehPacoteRetorno(const uint8_t* body, uint8_t len) {
  if (len == 51 && body[0] == 'T') return true; // StatusMsg da Casa -> Hub
  if (len == 5 && body[0] == 'N')  return true; // NonceResp da Casa -> Hub
  return false;
}

static const char* nomeComando(const Cap& c) {
  if (c.len == 1) {
    switch (c.body[0]) {
      case 'L': return "LIGAR (ON)";
      case 'D': return "APAGAR (OFF)";
      case 'A': return "ALARME";
      case 'S': return "SILENCIAR";
      case '?': return "REQ_NONCE";
    }
  }
  if (c.len == 5 && c.body[0] == 'N')  return "RESP_NONCE";
  if (c.len == 51 && c.body[0] == 'T') return "STATUS_CASA";
  if (c.len == 16) return "? (cifrado AES)";
  return "? (desconhecido)";
}

static void imprimeCap(const Cap& c) {
  char src[18];
  macStr(c.src, src);
  if (c.bcast) {
    Serial.printf("#%-3ld %7.1fs  BCAST                      de %s  %ddBm  %2dB  %-16s  ",
                  c.id, c.ms / 1000.0, src, c.rssi, c.len, nomeComando(c));
  } else {
    char dst[18];
    macStr(c.dst, dst);
    Serial.printf("#%-3ld %7.1fs  UNI -> %s   de %s  %ddBm  %2dB  %-16s  ",
                  c.id, c.ms / 1000.0, dst, src, c.rssi, c.len, nomeComando(c));
  }
  int show = c.len > 16 ? 16 : c.len;
  for (int i = 0; i < show; i++) Serial.printf("%02x", c.body[i]);
  if (c.len > show) Serial.print("...");
  Serial.println();
}

static Cap* buscaId(long id) {
  if (id < base || id > total || id <= total - HIST_N) return nullptr;
  Cap* c = &hist[id % HIST_N];
  return (c->id == id) ? c : nullptr;
}

static void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  (void)type;
  const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* p = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 40) return;

  if (p[24] != 0x7F) return;                                    // Action vendor-specific
  if (p[32] != 0xDD) return;                                    // Element vendor-specific
  if (p[34] != 0x18 || p[35] != 0xFE || p[36] != 0x34) return;  // OUI Espressif
  if (p[37] != 0x04) return;                                    // Tipo ESP-NOW

  if (varrendo) {
    conta_varre++;
    return;
  }

  const uint8_t* da = p + 4;
  const uint8_t* sa = p + 10;
  if (memcmp(sa, meu_mac, 6) == 0) return; // Ignora o próprio replay

  int bodylen = p[33] - 5;
  if (bodylen < 1) return;
  if (39 + bodylen > len) bodylen = len - 39;
  if (bodylen > BODY_MAX) bodylen = BODY_MAX;

  uint8_t next = (q_head + 1) % QN;
  if (next == q_tail) return;
  Cap& c = q[q_head];
  memcpy(c.src, sa, 6);
  memcpy(c.dst, da, 6);
  memcpy(c.body, p + 39, bodylen);
  c.len   = (uint8_t)bodylen;
  c.bcast = (memcmp(da, BROADCAST, 6) == 0);
  c.rssi  = pkt->rx_ctrl.rssi;
  c.ms    = millis();
  q_head  = next;
}

static void garantePeer(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

static void replay(long id, int vezes) {
  Cap* c = buscaId(id);
  if (!c) {
    Serial.printf("[Erro] Pacote #%ld não está no histórico (use 'l').\n", id);
    return;
  }
  char dst[18];
  macStr(c->dst, dst);
  garantePeer(c->dst);
  for (int i = 0; i < vezes; i++) {
    esp_err_t r = esp_now_send(c->dst, c->body, c->len);
    Serial.printf("[Replay #%ld] -> %s  %s  (%s)\n",
                  id, c->bcast ? "BROADCAST" : dst, nomeComando(*c),
                  r == ESP_OK ? "ok" : "falhou");
    ledPisca(2, 35);
    if (i + 1 < vezes) delay(250);
  }
}

static void mudaCanal(int n, bool gravar) {
  canal = (uint8_t)n;
  esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
  if (gravar) prefs.putUChar("canal", canal);
  Serial.printf("[Canal] Ajustado para canal %d%s.\n", canal, gravar ? " (salvo)" : "");
}

static void varre() {
  uint8_t original = canal;
  Serial.println("[Varredura] Contando pacotes ESP-NOW nos canais 1-13:");
  varrendo = true;
  for (int ch = 1; ch <= 13; ch++) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    conta_varre = 0;
    delay(VARRE_MS);
    uint32_t n = conta_varre;
    Serial.printf("  canal %2d: %3lu  ", ch, (unsigned long)n);
    for (uint32_t i = 0; i < n && i < 40; i++) Serial.print('#');
    Serial.println();
  }
  varrendo = false;
  esp_wifi_set_channel(original, WIFI_SECOND_CHAN_NONE);
  Serial.printf("[Varredura] Voltou ao canal %d (troque com: k <n>).\n", original);
}

static void ajuda() {
  Serial.println("Comandos: l=listar | <n>=replay #n | <n> xK=repetir K vezes | u=último");
  Serial.println("          a=mostrar/ocultar STATUS/NONCE | m=ao vivo | c=limpar | v=varrer | k <n>=canal");
  Serial.println("          Botão BOOT = replay do último pacote capturado.");
}

static void lista() {
  if (total < base) {
    Serial.println("(histórico vazio)");
    return;
  }
  long ini = total - HIST_N + 1;
  if (ini < base) ini = base;
  Serial.println("-------------------------------------------------------------------------------");
  for (long id = ini; id <= total; id++) {
    Cap* c = buscaId(id);
    if (c) imprimeCap(*c);
  }
  Serial.println("-------------------------------------------------------------------------------");
}

static void executa(char* s) {
  while (*s == ' ') s++;
  if (*s == 0) return;
  char k = (char)tolower((unsigned char)*s);
  if (k == 'l') { lista(); return; }
  if (k == 'h' || k == '?') { ajuda(); return; }
  if (k == 'u') {
    if (total >= base) replay(total, 1);
    else Serial.println("(nada capturado ainda)");
    return;
  }
  if (k == 'a') {
    mostrar_todos = !mostrar_todos;
    Serial.printf("[Filtro] Retornos STATUS/NONCE: %s\n", mostrar_todos ? "visíveis" : "ocultos");
    return;
  }
  if (k == 'm') {
    ao_vivo = !ao_vivo;
    Serial.printf("[Ao vivo] %s\n", ao_vivo ? "ligado" : "pausado");
    return;
  }
  if (k == 'c') { base = total + 1; Serial.println("[Histórico] Limpo."); return; }
  if (k == 'v') { varre(); return; }
  if (k == 'k') {
    int n = atoi(s + 1);
    if (n < 1 || n > 13) { Serial.println("Use um canal de 1 a 13 (ex.: k 1)"); return; }
    mudaCanal(n, true);
    return;
  }
  if (*s == '#') s++;
  if (isdigit((unsigned char)*s)) {
    long id = strtol(s, &s, 10);
    int vezes = 1;
    char* x = strchr(s, 'x');
    if (x) {
      vezes = atoi(x + 1);
      if (vezes < 1) vezes = 1;
      if (vezes > 20) vezes = 20;
    }
    replay(id, vezes);
    return;
  }
  Serial.printf("Comando desconhecido: %s\n", s);
  ajuda();
}

static char linha[32];
static uint8_t nlinha = 0;

static void leTerminal() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r' || ch == '\n') {
      if (nlinha) {
        Serial.println();
        linha[nlinha] = 0;
        executa(linha);
        nlinha = 0;
      }
    } else if ((ch == 8 || ch == 127) && nlinha) {
      nlinha--;
      Serial.print("\b \b");
    } else if (ch >= 32 && nlinha < sizeof(linha) - 1) {
      linha[nlinha++] = ch;
      Serial.print(ch);
    }
  }
}

void setup() {
  pinMode(PINO_LED, OUTPUT);
  ledSet(false);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  Serial.begin(115200);
  delay(300);

  configurarAntenaInterna();

  prefs.begin("sniffer", false);
  canal = prefs.getUChar("canal", CANAL_PADRAO);
  if (canal < 1 || canal > 13) canal = CANAL_PADRAO;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_get_mac(WIFI_IF_STA, meu_mac);
  esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() == ESP_OK) garantePeer(BROADCAST);

  wifi_promiscuous_filter_t filtro = {};
  filtro.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filtro);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);

  Serial.println("===============================================================================");
  Serial.println("Sniffer ESP-NOW (XIAO ESP32-C6) — Modo Monitor + Replay Verbatim");
  Serial.printf("Canal %d | Filtro de retornos STATUS/NONCE: ativo (use 'a' para ver todos)\n", canal);
  ajuda();
  Serial.println("===============================================================================");
  ledPisca(2, 120);
}

static bool botao_ant = false;

void loop() {
  while (q_tail != q_head) {
    Cap c = q[q_tail];
    q_tail = (q_tail + 1) % QN;

    if (!mostrar_todos && ehPacoteRetorno(c.body, c.len)) {
      continue;
    }

    c.id = ++total;
    hist[c.id % HIST_N] = c;
    if (ao_vivo) {
      if (nlinha) Serial.println();
      imprimeCap(c);
      if (nlinha) {
        linha[nlinha] = 0;
        Serial.print(linha);
      }
    }
    ledPisca(1, 25);
  }

  leTerminal();

  bool pressionado = (digitalRead(PINO_BOTAO) == LOW);
  if (pressionado && !botao_ant) {
    delay(20);
    if (digitalRead(PINO_BOTAO) == LOW && total >= base) {
      replay(total, 1);
    }
  }
  botao_ant = pressionado;

  delay(5);
}
