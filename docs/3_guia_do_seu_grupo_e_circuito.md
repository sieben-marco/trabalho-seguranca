# Guia do Grupo: Circuito, Alimentação e Roteiro das 4 Etapas (10 Minutos)

Este documento é o manual prático completo para montar as **3 placas Seeed Studio XIAO ESP32-C6** na protoboard (incluindo o esquema de alimentação usando apenas **1 cabo USB-C + 1 cabo Micro-USB**), gravar os firmwares e conduzir a gravação/apresentação de **10 minutos**.

---

## 1. Ligação dos Pinos na Protoboard

### 1.1. XIAO #1 — Casa Inteligente (LEDs na mesma fileira do `GND`)

Na XIAO ESP32-C6 (lado esquerdo visto pelo verso, onde ficam `VBUS`, `GND`, `3V3`, `D10`, `D9`):

| Componente | Pino na XIAO #1 | GPIO no ESP32-C6 | Função |
| :--- | :--- | :--- | :--- |
| **LED Azul** (Anodo / perna maior) | **`D10`** | `GPIO 18` | **Luz Principal** (inicia acesa) |
| **LED Vermelho** (Anodo / perna maior) | **`D9`** | `GPIO 20` | **Alarme de Segurança** (pisca a cada **700 ms** até login válido do `admin`) |
| **Catodos dos 2 LEDs** (pernas menores) | **`GND`** *(via 1 resistor 220Ω–470Ω)* | `GND` | Terra comum na protoboard |

- O firmware da Casa faz **multiplexação por divisão de tempo a 125 Hz** (`Ticker`, *break-before-make*, 75% Azul / 25% Vermelho na sobreposição). Assim os dois LEDs funcionam juntos com **apenas 1 resistor compartilhado**, sem que o LED vermelho "roube" a corrente do azul.

### 1.2. Alimentação das 3 Placas com Apenas 1 Cabo USB-C (Usando a ESP8266 como Fonte 5V)

Como a **XIAO #3 (Sniffer)** não possui pinagem soldada e precisa ficar ligada ao notebook pelo único **cabo USB-C** para exibir o Monitor Serial, usamos a **NodeMCU ESP8266 LoLin V3** ligada no cabo **Micro-USB** exclusivamente como derivação de **5V** para alimentar a **XIAO #1** e a **XIAO #2**:

```text
[Notebook ou Carregador 5V] ──Micro-USB──▶ [NodeMCU ESP8266 LoLin V3]
                                             ├── EN (12º pino esq.) ──▶ G (14º pino esq.) [desliga o chip ESP8266]
                                             ├── VU (3º pino esq., 5V) ──▶ barramento (+) ──▶ VBUS da XIAO #1 e VBUS da XIAO #2
                                             └── G  (2º pino esq., GND) ─▶ barramento (-) ──▶ GND da XIAO #1 e GND da XIAO #2

[Notebook] ──Cabo USB-C (único)──▶ [XIAO #3 Sniffer (sem pinos)] ── Monitor Serial 115200 baud
```

**Conferência dos pinos na NodeMCU ESP8266 LoLin V3** *(com a antena no topo e o Micro-USB embaixo — todos na fileira esquerda, a do `A0`)*:

| Posição (de cima para baixo) | Serigrafia | Ligação | Motivo |
| :--- | :--- | :--- | :--- |
| **2º pino** | **`G`** | Barramento `-` → `GND` de XIAO #1 e XIAO #2 | Referência de terra comum |
| **3º pino** | **`VU`** | Barramento `+` → `VBUS` de XIAO #1 e XIAO #2 | Entrega ~5V direto do cabo Micro-USB para o regulador interno de cada XIAO (até 700 mA) |
| **12º pino** ↔ **14º pino** *(ou 10º)* | **`EN` ↔ `G`** | Jumper curto entre `EN` e `G` | Coloca o chip ESP8266 em *shutdown* (~0,33 mA pelo pull-up de 10 kΩ), evitando desperdício de corrente e interferência Wi-Fi no Canal 1 |

> [!CAUTION]
> **Regras de segurança elétrica na bancada:**
> 1. **Sempre alimente via `VBUS` (5V)** e nunca puxando `3V3` de uma placa para outra.
> 2. **Nunca encoste `VU` em `G`** (curto direto na porta USB).
> 3. **NUNCA conecte o cabo USB-C na XIAO #1 ou na XIAO #2 enquanto o jumper de `VBUS` estiver plugado na protoboard!** Desconecte sempre o fio `VBUS` (ou retire a XIAO da protoboard) antes de conectá-la ao notebook para gravar um sketch.

> [!TIP]
> **Dica para o vídeo (Etapas 3 e 4):** para demonstrar o corte de energia/reinicialização da **Casa (`XIAO #1`)** sem derrubar o Hub (`XIAO #2`) nem o Sniffer (`XIAO #3`), basta desconectar e reconectar o jumper **`VBUS`** da XIAO #1 (ou apertar o botão físico `R` nela)!

---

## 2. Preparação Única Antes de Gravar o Vídeo

1. Na Arduino IDE, confirme o pacote **`esp32 by Espressif Systems` (3.x)** e selecione a placa **`Seeed Studio XIAO ESP32C6`**.
2. **Descubra os MACs** (uma placa por vez no cabo USB-C, sem `VBUS` externo):
   - Grave `firmware/0_mostra_mac/0_mostra_mac.ino` na **XIAO #1 (Casa)** e copie o array `{ 0x.., ... }` do Monitor Serial (`115200`).
   - Grave `firmware/0_mostra_mac/0_mostra_mac.ino` na **XIAO #2 (Hub Web)** e copie o array `{ 0x.., ... }`.
3. **Preencha os MACs** no topo dos sketches das Etapas 2, 3 e 4:
   - `HUB_MAC[6]` em `etapa2_casa_aes.ino`, `etapa3_casa_contador.ino` e `etapa4_casa_nonce.ino`.
   - `CASA_MAC[6]` em `etapa2_hub_web_aes.ino`, `etapa3_hub_web_contador.ino` e `etapa4_hub_web_nonce.ino`.
   *(A Etapa 1 usa Broadcast e não precisa editar MAC.)*
4. **Grave o Sniffer uma única vez:** suba `firmware/sniffer_espnow/sniffer_espnow.ino` na **XIAO #3** (a sem pinos) e deixe-a conectada no cabo USB-C com o Monitor Serial aberto em `115200 baud`. Ela não precisa mais ser regravada em nenhuma etapa!

---

## 3. Roteiro do Vídeo / Apresentação (Total: ~10 Minutos)

Conecte o celular (ou notebook) no Wi-Fi criado pela XIAO #2:
- **SSID:** `SmartHome_IoT` | **Senha:** `12345678`
- **Convidado:** `http://192.168.4.1/`
- **Admin:** `http://192.168.4.1/admin` *(usuário `admin`, senha `123`)*

### Etapa 1 — Sistema Inseguro: Falta do `return;` + Texto Claro (~1,5 min)
- **Sketches:** `etapa1_casa` (XIAO #1) + `etapa1_hub_web` (XIAO #2).
- **O que mostrar:**
  1. Mostre a página de Convidado (`/`): ela exibe apenas **Ligar Luz** (sem botão de apagar).
  2. Mostre no terminal do Sniffer (`XIAO #3`) que comandos enviados pelo Hub aparecem em **texto claro** (`BCAST 1B LIGAR (ON)` / `APAGAR (OFF)`).
  3. Execute pelo terminal (ou barra de endereço) sem senha de admin:
     ```bash
     curl "http://192.168.4.1/luz?acao=OFF&user=visitante"
     ```
  4. **Resultado:** o servidor devolve HTTP `403`, o **LED Vermelho começa a piscar (700 ms)** — **mas o LED Azul APAGA mesmo assim** porque faltou um `return;` dentro do `if (user != ADMIN_USER)` no Hub Web (*Mistake*, Equipe T6 do artigo CCS '22)!
  5. Faça login em `/admin` (`admin` / `123`) para mostrar o alarme silenciando e reacenda a luz.

### Etapa 2 — `return;` Corrigido + ESP-NOW Unicast AES-128 (~2 min)
- **Sketches:** `etapa2_casa_aes` (XIAO #1) + `etapa2_hub_web_aes` (XIAO #2). *(Lembre de desconectar `VBUS` antes de gravar!)*
- **O que mostrar:**
  1. Repita `curl "http://192.168.4.1/luz?acao=OFF&user=visitante"` → agora o **LED Azul permanece aceso** e apenas o **LED Vermelho pisca** (falha de fluxo corrigida!).
  2. Faça login no celular em `/admin` (alarme silencia), clique em **Apagar Luz** e depois em **Ligar Luz**.
  3. Olhe o terminal do Sniffer (`XIAO #3`): agora aparece `UNI -> ... 16B ? (cifrado AES) 3fa1c9...`. **O Sniffer não consegue ler o comando!**
  4. **O Ataque (Replay Verbatim):** digite no Monitor Serial do Sniffer o número do pacote correspondente ao *Apagar Luz* (ex.: `2` + Enter).
  5. **Resultado:** mesmo sem entender o conteúdo cifrado, a Casa (`XIAO #1`) decifra os bytes reenviados e **APAGA o LED Azul** — e a tela do celular atualiza sozinha para `"Luz apagada"`!
  6. **Lição:** criptografia dá **confidencialidade**, mas **não** impede Replay (*Misunderstanding*).

### Etapa 3 — AES-128 + Contador Sequencial em RAM (~2,5 min)
- **Sketches:** `etapa3_casa_contador` (XIAO #1) + `etapa3_hub_web_contador` (XIAO #2).
- **O que mostrar:**
  1. No Sniffer, digite `c` + Enter para limpar o histórico.
  2. Pelo celular (`/admin`), clique em **Apagar Luz** (`seq` menor) e depois em **Ligar Luz** (`seq` maior).
  3. No Sniffer, tente dar **Replay** no pacote do *Apagar Luz* (ex.: `1` + Enter):
     - **Resultado 1:** **BLOQUEADO!** A Casa vê `seq <= ultimoSeq`, mantém o **LED Azul aceso** e dispara o **LED Vermelho piscando (700 ms)**!
  4. **A Reviravolta (*Reset-to-Zero Replay*):** na frente da câmera, **puxe e recoloque o jumper `VBUS` (ou aperte RESET)** da **XIAO #1 (Casa)**! Ela religa com LED Azul aceso e `ultimoSeq = 0` na RAM.
  5. No Sniffer, digite novamente o **mesmo número de pacote** (ex.: `1` + Enter):
     - **Resultado 2:** como `seq > 0`, a Casa **ACEITA o Replay antigo e APAGA o LED Azul!**
  6. **Lição:** contador em memória volátil falha sob queda de energia ou reinício forçado (*Unintuitive* / estado volátil).

### Etapa 4 — AES-128 + Nonce Aleatório Desafio-Resposta (~2 min)
- **Sketches:** `etapa4_casa_nonce` (XIAO #1) + `etapa4_hub_web_nonce` (XIAO #2).
- **O que mostrar:**
  1. No Sniffer, limpe o histórico (`c` + Enter).
  2. Pelo celular (`/admin`), clique em **Apagar Luz** e depois em **Ligar Luz**. No Sniffer aparecem `REQ_NONCE` (`?`) seguido do bloco de `16B ? (cifrado AES)`.
  3. Tente dar **Replay** no bloco cifrado do *Apagar Luz* → **BLOQUEADO!** (o Nonce já foi queimado; LED Vermelho pisca a 700 ms).
  4. Agora **puxe e recoloque o jumper `VBUS` (ou aperte RESET)** da **XIAO #1 (Casa)** novamente!
  5. Tente o **mesmo Replay** no Sniffer → **CONTINUA BLOQUEADO!** Ao religar, `nonceAtivo` nasce `false` e qualquer novo desafio é sorteado por hardware via `esp_random()`. O **LED Vermelho** dispara piscando (700 ms) e **só para quando o Admin faz login válido** em `/admin`!

### Fechamento Teórico (~2 min)
- Conecte os 4 passos com a taxonomia do artigo de Fulton et al. (CCS '22):
  - **Etapa 1** = *Mistake* (fácil de corrigir com 1 linha `return;`);
  - **Etapa 2** = *Misunderstanding* (achar que criptografia sozinha impede reenvio);
  - **Etapa 3** = *Unintuitive* / caso de borda (contador volátil quebrado por reinício);
  - **Etapa 4** = protocolo seguro completo (RBAC + AES-128 + Nonce aleatório por hardware).

---

## 4. Cola Rápida de Comandos do Sniffer (`XIAO #3` — 115200 baud)

| Comando | Ação |
| :--- | :--- |
| `l` | Lista o histórico numerado de pacotes capturados |
| `<n>` | Faz **Replay verbatim** do pacote `#n` (ex.: `1`) |
| `<n> x3` | Repete o pacote `#n` três vezes |
| `u` | Repete o último pacote capturado *(mesmo efeito do botão físico `BOOT`)* |
| `c` | Limpa o histórico (útil entre uma etapa e outra no vídeo) |
| `a` | Mostra/oculta pacotes de retorno (`STATUS_CASA` e `RESP_NONCE`) |
| `m` | Pausa/retoma a impressão ao vivo |
| `v` / `k <n>` | Varre canais 1–13 / muda o canal do rádio |
