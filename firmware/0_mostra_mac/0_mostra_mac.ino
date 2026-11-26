/*
 * ============================================================================
 * Utilitário: Descobrir o endereço MAC STA da Seeed Studio XIAO ESP32-C6
 * ============================================================================
 * Placa na Arduino IDE: Seeed Studio XIAO ESP32C6 (core esp32 by Espressif 3.x)
 * Monitor Serial: 115200 baud
 *
 * Grave este sketch uma vez na XIAO #1 (Casa) e na XIAO #2 (Hub Web) para
 * anotar o MAC de cada placa no formato pronto para colar nas Etapas 2, 3 e 4.
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_wifi.h>

static uint8_t meuMac[6] = {0};

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

static void imprimirMac() {
  Serial.println("\n----------------------------------------------------");
  Serial.printf("MAC STA (legível): %02x:%02x:%02x:%02x:%02x:%02x\n",
                meuMac[0], meuMac[1], meuMac[2], meuMac[3], meuMac[4], meuMac[5]);
  Serial.printf("Formato para colar no código:\n  { 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X }\n",
                meuMac[0], meuMac[1], meuMac[2], meuMac[3], meuMac[4], meuMac[5]);
  Serial.println("----------------------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  configurarAntenaInterna();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_get_mac(WIFI_IF_STA, meuMac);

  imprimirMac();
}

void loop() {
  delay(3000);
  imprimirMac();
}
