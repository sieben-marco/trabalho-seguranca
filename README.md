# Smart Home IoT: Segurança em 4 Etapas com 3× XIAO ESP32-C6 e ESP-NOW

Demonstração prática (~10 minutos em vídeo + explicação) baseada no artigo [*Understanding the How and the Why: Exploring Secure Development Practices through a Course Competition*](docs/Understanding%20the%20How%20and%20the%20Why%20-%20Exploring%20Secure%20Development%20Practices%20through%20a%20Course%20Competition.pdf) (Fulton et al., ACM CCS '22) e na metodologia **BIBIFI (*Build It, Break It, Fix It*)**, construída com **3 placas Seeed Studio XIAO ESP32-C6** comunicando via **ESP-NOW**.

---

## Arquitetura do Projeto (2 Saltos: Hub Web + Rádio ESP-NOW)

```text
[Celular / Notebook] ──HTTP (Wi-Fi SoftAP Canal 1)──▶ [XIAO #2: Hub Web] ══(ESP-NOW Canal 1)══▶ [XIAO #1: Casa / LEDs]
                                                             ▲                    │
                                                             └─── Status ESP-NOW ─┤
                                                                                  ▼ (modo monitor promíscuo)
                                                                       [XIAO #3: Sniffer C++]
                                                                     Captura e faz Replay verbatim
```

- **XIAO #1 (Casa / Protoboard):** controla o **LED Azul (`D10`)** (luz principal) e o **LED Vermelho (`D9`)** (alarme intermitente a cada 700 ms, silenciado apenas após login válido do administrador), multiplexados a 125 Hz para funcionar com **1 único resistor compartilhado**.
- **XIAO #2 (Hub Web):** sobe o Wi-Fi `SmartHome_IoT` (senha `12345678`, Canal 1, `http://192.168.4.1/`) com a interface web (`web.h`) para Convidado (`/`) e Administrador (`/admin`, usuário `admin`, senha `123`), traduzindo os cliques web em pacotes **ESP-NOW** para a Casa (`#1`).
- **XIAO #3 (Sniffer):** gravada **uma única vez** em modo monitor (`esp_wifi_set_promiscuous`), captura todos os quadros ESP-NOW no Canal 1 (broadcast, unicast ou cifrados) e executa **Replay Attack verbatim** pelo Monitor Serial (`115200 baud`) ou pelo botão `BOOT`.

### As 4 Etapas Progressivas

1. **Etapa 1 — Insegura (`etapa1_casa` + `etapa1_hub_web`):** ESP-NOW em Broadcast/texto claro + erro de fluxo (*Mistake*, Equipe T6 do artigo) — falta `return;` ao negar `OFF` para visitante no Hub Web, disparando o alarme **e** apagando a luz azul.
2. **Etapa 2 — AES-128 Unicast (`etapa2_casa_aes` + `etapa2_hub_web_aes`):** `return;` corrigido + rádio cifrado em **AES-128** com assinatura interna `"SH26"` (*Misunderstanding*) — o Sniffer não consegue ler os 16 bytes cifrados, mas faz **Replay verbatim** e apaga a luz.
3. **Etapa 3 — AES-128 + Contador em RAM (`etapa3_casa_contador` + `etapa3_hub_web_contador`):** exige `seq > ultimoSeq` dentro do bloco AES — Replay normal é bloqueado (alarme pisca), mas ao **reiniciar / cortar a energia da Casa (`#1`)**, `ultimoSeq` volta a `0` e o **mesmo Replay antigo volta a apagar a luz** (*Reset-to-Zero Replay*).
4. **Etapa 4 — AES-128 + Nonce Aleatório (`etapa4_casa_nonce` + `etapa4_hub_web_nonce`):** desafio-resposta com `esp_random()` (hardware) e validade de 3 s — imune a Replay normal **e** imune a reinicialização da Casa (`#1`).

---

## Mapa de Arquivos

| Pasta / Arquivo | Função |
| :--- | :--- |
| [`docs/3_guia_do_seu_grupo_e_circuito.md`](docs/3_guia_do_seu_grupo_e_circuito.md) | Guia prático completo: pinagem na protoboard, alimentação com 1 cabo USB-C, comandos do Sniffer e roteiro minuto a minuto das 4 etapas. |
| [`docs/1_analise_artigo_e_fundamentacao.md`](docs/1_analise_artigo_e_fundamentacao.md) | Análise das RQs do artigo (Fulton et al., CCS '22) e conexão teórica com as 4 etapas em ESP-NOW. |
| [`firmware/0_mostra_mac/`](firmware/0_mostra_mac/) | Utilitário para ler o endereço MAC STA da XIAO #1 e da XIAO #2 já formatado para colar no código. |
| [`firmware/sniffer_espnow/`](firmware/sniffer_espnow/) | **XIAO #3 (Sniffer):** grava uma única vez e serve para todas as 4 etapas. |
| [`firmware/etapa1_casa/`](firmware/etapa1_casa/) e [`firmware/etapa1_hub_web/`](firmware/etapa1_hub_web/) | **Etapa 1:** falta de `return;` no Hub Web + ESP-NOW texto claro. |
| [`firmware/etapa2_casa_aes/`](firmware/etapa2_casa_aes/) e [`firmware/etapa2_hub_web_aes/`](firmware/etapa2_hub_web_aes/) | **Etapa 2:** `return;` corrigido + ESP-NOW Unicast AES-128 (vulnerável a Replay). |
| [`firmware/etapa3_casa_contador/`](firmware/etapa3_casa_contador/) e [`firmware/etapa3_hub_web_contador/`](firmware/etapa3_hub_web_contador/) | **Etapa 3:** AES-128 + contador `seq` em RAM (vulnerável a Reset-to-Zero Replay). |
| [`firmware/etapa4_casa_nonce/`](firmware/etapa4_casa_nonce/) e [`firmware/etapa4_hub_web_nonce/`](firmware/etapa4_hub_web_nonce/) | **Etapa 4:** AES-128 + Nonce aleatório desafio-resposta (seguro). |
| [`firmware/legado_esp8266/`](firmware/legado_esp8266/) | Versão anterior completa feita para 1× NodeMCU ESP8266 em HTTP puro (mantida como legado). |
| [`ataques/`](ataques/) | Scripts Python de apoio (`painel_ataque_grupo.py` e `cliente_admin.py`, usados com o legado e úteis para testar a Etapa 1). |
| [`docs/2_enunciado_para_os_colegas.md`](docs/2_enunciado_para_os_colegas.md) | Enunciado de referência da dinâmica BIBIFI original. |

---

## Montagem de Hardware e Alimentação (1 Cabo USB-C + 1 Cabo Micro-USB)

### 1. Pinagem dos LEDs na XIAO #1 (Casa — tudo na fileira lateral do `GND`)

Na **Seeed Studio XIAO ESP32-C6**, todos os pinos da Casa ficam do mesmo lado da placa (fileira de `VBUS`, `GND`, `3V3`, `D10`, `D9`), sem cruzar fios:

- **LED Azul (`D10` / GPIO 18):** Anodo (perna maior) — Luz Principal (inicia **acesa**).
- **LED Vermelho (`D9` / GPIO 20):** Anodo (perna maior) — Alarme de Segurança (pisca a cada **700 ms** após tentativa bloqueada, ataque ou login inválido e só apaga após login válido do administrador).
- **Catodos dos 2 LEDs (pernas menores):** ligados juntos passando por **1 único resistor (220Ω a 470Ω)** até o **`GND`** da XIAO #1 (o firmware multiplexa os LEDs a 125 Hz via `Ticker` para ambos funcionarem juntos com apenas 1 resistor).

### 2. Como Alimentar as 3 Placas Tendo Apenas 1 Cabo USB-C

Como a **XIAO #3 (Sniffer)** não tem pinos soldados e precisa ficar conectada ao notebook pelo **único cabo USB-C** para exibir o Monitor Serial, usamos a **NodeMCU ESP8266 LoLin V3** (com o cabo **Micro-USB**) apenas como fonte 5V na protoboard para alimentar as **XIAO #1** e **XIAO #2**:

- **Na NodeMCU ESP8266 LoLin V3** *(antena no topo, Micro-USB embaixo — todos na fileira esquerda, a do `A0`)*:
  - **2º pino (`G`)** → barramento `-` da protoboard → **`GND`** da XIAO #1 e **`GND`** da XIAO #2.
  - **3º pino (`VU` — 5V direto da USB)** → barramento `+` da protoboard → **`VBUS`** da XIAO #1 e **`VBUS`** da XIAO #2.
  - **12º pino (`EN`) ↔ 14º pino (`G`)** *(jumper curto)*: desliga completamente o chip ESP8266 (modo *shutdown*, ~0,33 mA pelo pull-up interno de 10 kΩ, zero interferência Wi-Fi no Canal 1), mantendo os 5V ativos no pino `VU`.
- **Na XIAO #3 (Sniffer):** conectada direto ao notebook via **cabo USB-C**.

> [!CAUTION]
> **Regras obrigatórias de proteção elétrica:**
> 1. **Nunca** encoste `VU` em `G` (curto direto nos 5V da USB).
> 2. **Nunca** plugue o cabo **USB-C** na XIAO #1 ou na XIAO #2 enquanto o jumper vindo de `VU` estiver conectado no `VBUS` dela. Sempre desconecte o jumper de `VBUS` (ou retire a placa da protoboard) antes de conectar o cabo USB-C para gravar uma nova etapa!

---

## Como Gravar na Arduino IDE

- **Pacote de placas:** `esp32 by Espressif Systems` (versão **3.x**).
- **Placa selecionada:** `Seeed Studio XIAO ESP32C6`.
- **Bibliotecas:** apenas bibliotecas nativas do core ESP32 (`WiFi.h`, `WebServer.h`, `MD5Builder.h`, `Ticker.h`, `esp_wifi.h`, `esp_now.h`, `mbedtls/aes.h`, `Preferences.h`).
- **Monitor Serial:** `115200 baud`.
- **Passo inicial único:** grave `firmware/0_mostra_mac/0_mostra_mac.ino` na **XIAO #1** e na **XIAO #2**, anote os dois endereços MAC e preencha `CASA_MAC` (nos sketches `etapa2/3/4_hub_web_*`) e `HUB_MAC` (nos sketches `etapa2/3/4_casa_*`). Depois grave `firmware/sniffer_espnow/sniffer_espnow.ino` na **XIAO #3** uma única vez.
