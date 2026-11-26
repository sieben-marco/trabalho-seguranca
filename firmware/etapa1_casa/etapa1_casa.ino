/*
 * ============================================================================
 * Etapa 1 — XIAO #1 (Casa / Protoboard): ESP-NOW Texto Claro (Broadcast)
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 *
 * Pinagem na protoboard (tudo na fileira lateral do GND):
 *   - D10 (GPIO 18) -> Anodo do LED Azul (Luz Principal - inicia acesa)
 *   - D9  (GPIO 20) -> Anodo do LED Vermelho (Alarme intermitente 700 ms)
 *   - GND           -> Catodos dos 2 LEDs juntos passando por 1 resistor comum
 *
 * Multiplexação em software via Ticker (125 Hz, 75% Azul / 25% Vermelho na
 * sobreposição) para manter ambos os LEDs visíveis com apenas 1 resistor.
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Ticker.h>
#include <string.h>

static const uint8_t CANAL_WIFI = 1;
static const uint8_t BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// --- Pinagem na mesma fileira do GND da XIAO ESP32-C6 ---
static const int PIN_LUZ_AZUL    = D10; // GPIO 18
static const int PIN_ALARME_VERM = D9;  // GPIO 20

// --- Multiplexação (1 resistor compartilhado) e Alarme Intermitente ---
static const unsigned long INTERVALO_PISCA_ALARME_MS = 700; // 700 ms aceso / 700 ms apagado
static const unsigned long INTERVALO_MUX_MS          = 2;   // 2 ms/slot -> 4 slots = 8 ms (125 Hz)
static const uint8_t       SLOTS_MUX                 = 4;   // 3 slots Azul (75%) + 1 slot Vermelho (25%)

// --- Códigos de comando ESP-NOW (Etapa 1: 1 byte em texto claro) ---
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

static void atualizarSaidaLeds() {
  bool querAzul = luzAcesa;
  bool querVermelho = alarmeAtivo &&
    (((millis() - alarmeInicioMs) / INTERVALO_PISCA_ALARME_MS) % 2 == 0);

  // Break-before-make: nunca deixar D10 e D9 em HIGH ao mesmo tempo
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
  esp_now_send(BROADCAST, (const uint8_t*)&st, sizeof(st));
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
  if (pkt.len != 1) return; // Ignora pacotes que não sejam comandos de 1 byte

  uint8_t cmd = pkt.data[0];
  if (cmd == CMD_ON) {
    luzAcesa = true;
    Serial.println("[Etapa 1 - Casa] Comando texto claro recebido: LIGAR");
    enviarStatus("Luz acesa (Etapa 1 - texto claro)");
  } else if (cmd == CMD_OFF) {
    luzAcesa = false;
    Serial.println("[Etapa 1 - Casa] Comando texto claro recebido: APAGAR");
    enviarStatus("Luz apagada (Etapa 1 - texto claro)");
  } else if (cmd == CMD_ALARME) {
    dispararLedAlarme();
    Serial.println("[Etapa 1 - Casa] Alarme disparado (LED vermelho piscando 700 ms)");
    enviarStatus("[Alerta] Tentativa não autorizada detectada");
  } else if (cmd == CMD_SILENCIAR) {
    if (alarmeAtivo) {
      desligarLedAlarme();
      Serial.println("[Etapa 1 - Casa] Alarme silenciado após login do admin");
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
  memcpy(peer.peer_addr, BROADCAST, 6);
  peer.channel = 0;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  esp_now_register_recv_cb(onEspNowRecv);

  Serial.println("\n[Etapa 1 - Casa] Pronta (ESP-NOW texto claro, Canal 1).");
  enviarStatus("Casa online (Etapa 1 - texto claro)");
}

void loop() {
  while (rxTail != rxHead) {
    RxPkt pkt = rxQueue[rxTail];
    rxTail = (rxTail + 1) % RX_Q_N;
    processarPacote(pkt);
  }
  delay(2);
}
