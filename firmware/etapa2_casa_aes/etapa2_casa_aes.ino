/*
 * ============================================================================
 * Etapa 2 — XIAO #1 (Casa / Protoboard): ESP-NOW Unicast + AES-128
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 *
 * Pinagem (tudo na fileira lateral do GND):
 *   - D10 (GPIO 18) -> Anodo do LED Azul (Luz Principal)
 *   - D9  (GPIO 20) -> Anodo do LED Vermelho (Alarme intermitente 700 ms)
 *   - GND           -> Catodos juntos via 1 resistor compartilhado
 *
 * Segurança ativa:
 *   - Pacotes de 16 bytes cifrados com AES-128 + assinatura interna "SH26".
 *   - Pacotes em texto claro ou forjados sem a chave são bloqueados com alarme.
 *
 * Vulnerabilidade didática (Misunderstanding):
 *   - Não há verificação de frescor (contador/nonce): o Sniffer (#3) não lê o
 *     comando, mas consegue fazer Replay verbatim dos 16 bytes e apagar a luz!
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <mbedtls/aes.h>
#include <Ticker.h>
#include <string.h>

// ====================== CONFIGURAÇÃO (PREENCHA O MAC) ======================
static const uint8_t CANAL_WIFI = 1;
// Cole aqui o MAC STA da XIAO #2 (Hub Web), descoberto com 0_mostra_mac.ino:
static uint8_t HUB_MAC[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02 };

// Chave AES-128 (16 bytes) — deve ser IDÊNTICA na XIAO #2 (Hub Web)
static const uint8_t CHAVE_AES[16] = {
  'S','m','a','r','t','H','o','m','e','2','0','2','6','K','e','y'
};
static const uint8_t MAGIC_TAG[4] = { 'S', 'H', '2', '6' };
// ===========================================================================

static const int PIN_LUZ_AZUL    = D10; // GPIO 18
static const int PIN_ALARME_VERM = D9;  // GPIO 20

static const unsigned long INTERVALO_PISCA_ALARME_MS = 700;
static const unsigned long INTERVALO_MUX_MS          = 2;
static const uint8_t       SLOTS_MUX                 = 4;

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
  uint8_t src[6];
  uint8_t data[64];
  uint8_t len;
};

static const uint8_t RX_Q_N = 12;
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static RxPkt rxQueue[RX_Q_N];

static Ticker muxTicker;
static volatile bool luzAcesa = true;
static volatile bool alarmeAtivo = false;
static volatile unsigned long alarmeInicioMs = 0;
static uint8_t slotMux = 0;

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

static void aesDecifrar16(const uint8_t in[16], uint8_t out[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_dec(&ctx, CHAVE_AES, 128);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, in, out);
  mbedtls_aes_free(&ctx);
}

static void atualizarSaidaLeds() {
  bool querAzul = luzAcesa;
  bool querVermelho = alarmeAtivo &&
    (((millis() - alarmeInicioMs) / INTERVALO_PISCA_ALARME_MS) % 2 == 0);

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

static void dispararLedAlarme() {
  if (!alarmeAtivo) {
    alarmeInicioMs = millis();
  }
  alarmeAtivo = true;
}

static void desligarLedAlarme() {
  alarmeAtivo = false;
}

static void enviarStatus(const char* evento) {
  StatusMsg st = {};
  st.tipo = 'T';
  st.luzAcesa = luzAcesa ? 1 : 0;
  st.alarmeAtivo = alarmeAtivo ? 1 : 0;
  strncpy(st.evento, evento, sizeof(st.evento) - 1);
  esp_now_send(HUB_MAC, (const uint8_t*)&st, sizeof(st));
}

static void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!info || !data || len <= 0) return;
  uint8_t next = (rxHead + 1) % RX_Q_N;
  if (next == rxTail) return;
  RxPkt& pkt = rxQueue[rxHead];
  memcpy(pkt.src, info->src_addr, 6);
  uint8_t copyLen = len > (int)sizeof(pkt.data) ? (uint8_t)sizeof(pkt.data) : (uint8_t)len;
  memcpy(pkt.data, data, copyLen);
  pkt.len = copyLen;
  rxHead = next;
}

static void processarPacote(const RxPkt& pkt) {
  if (pkt.len != 16) {
    dispararLedAlarme();
    Serial.printf("[Bloqueado] Pacote não cifrado (%uB) rejeitado.\n", pkt.len);
    enviarStatus("[Bloqueado] Pacote fora do formato AES-128");
    return;
  }

  uint8_t claro[16];
  aesDecifrar16(pkt.data, claro);

  // Layout Etapa 2: [0]=cmd | [1..4]="SH26" | [5..15]=aleatório
  if (memcmp(claro + 1, MAGIC_TAG, 4) != 0) {
    dispararLedAlarme();
    Serial.println("[Bloqueado] Assinatura AES inválida (pacote adulterado/forjado).");
    enviarStatus("[Bloqueado] Pacote AES adulterado ou sem chave");
    return;
  }

  // Vulnerável a Replay: aceita qualquer bloco AES válido sem checar frescor!
  uint8_t cmd = claro[0];
  if (cmd == CMD_ON) {
    luzAcesa = true;
    Serial.println("[Etapa 2 - Casa] Comando AES válido: LIGAR");
    enviarStatus("Luz acesa (AES-128 válido)");
  } else if (cmd == CMD_OFF) {
    luzAcesa = false;
    Serial.println("[Etapa 2 - Casa] Comando AES válido: APAGAR");
    enviarStatus("Luz apagada (AES-128 válido)");
  } else if (cmd == CMD_ALARME) {
    dispararLedAlarme();
    Serial.println("[Etapa 2 - Casa] Alarme disparado pelo Hub");
    enviarStatus("[Alerta] Tentativa bloqueada no Hub");
  } else if (cmd == CMD_SILENCIAR) {
    if (alarmeAtivo) {
      desligarLedAlarme();
      Serial.println("[Etapa 2 - Casa] Alarme silenciado após login do admin");
      enviarStatus("Alarme silenciado pelo administrador");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);

  pinMode(PIN_LUZ_AZUL, OUTPUT);
  pinMode(PIN_ALARME_VERM, OUTPUT);
  atualizarSaidaLeds();
  muxTicker.attach_ms(INTERVALO_MUX_MS, atualizarSaidaLeds);

  configurarAntenaInterna();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[Erro] Falha ao inicializar ESP-NOW");
    return;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, HUB_MAC, 6);
  peer.channel = 0;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  esp_now_register_recv_cb(onEspNowRecv);

  Serial.println("\n[Etapa 2 - Casa] Pronta (ESP-NOW Unicast + AES-128, vulnerável a Replay).");
  enviarStatus("Casa online (Etapa 2 - AES-128)");
}

void loop() {
  while (rxTail != rxHead) {
    RxPkt pkt = rxQueue[rxTail];
    rxTail = (rxTail + 1) % RX_Q_N;
    processarPacote(pkt);
  }
  delay(2);
}
