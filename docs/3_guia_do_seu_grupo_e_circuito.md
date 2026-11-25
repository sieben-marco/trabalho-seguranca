# Guia Secreto do Seu Grupo (Aplicadores / Red Team)

Este documento é o manual de bolso do seu grupo para montar o circuito na protoboard, conduzir o tempo da atividade (30 a 45 minutos) e executar os ataques ao vivo na frente da turma e do professor.

---

## 1. Montagem do Circuito (Apenas NodeMCU + LEDs)

Baseado nas fotos da sua bancada, você **não precisa** do Relé nem do Sensor de Som. Vamos usar apenas a **NodeMCU ESP8266** e **2 LEDs** (com resistores de 220Ω a 470Ω):

| Componente | Pino na NodeMCU (Serigrafia) | GPIO no Código | Função na Atividade |
| :--- | :--- | :--- | :--- |
| **LED Azul** (Anodo / perna maior) | Pino **`D1`** | `GPIO 5` | **Luz Principal da Casa Inteligente** (começa acesa; seu objetivo é apagá-la) |
| **LED Vermelho** (Anodo / perna maior) | Pino **`D2`** | `GPIO 4` | **Alarme de Segurança** (acende por 3 segundos quando uma tentativa é bloqueada e apaga) |
| **Catodos dos LEDs** (perna menor) | Pino **`G` (GND)** *(via resistor)* | `GND` | Terra comum na protoboard |

> [!TIP]
> **E se o seu grupo usar uma segunda NodeMCU como "Placa Atacante"?**
> A segunda NodeMCU (`4_placa_atacante_redteam.ino`) **não precisa de nenhum fio externo**! Ela usa o próprio **LED interno da placa** (`LED_BUILTIN` / `D4`) para indicar que conectou no Wi-Fi dos colegas e usa o próprio botão físico **`FLASH`** (que já vem soldado na NodeMCU, pino `D3` / `GPIO 0`) para disparar o ataque quando você apertar!

---

## 2. Cronograma Minuto a Minuto (Total: 35 a 45 min)

| Tempo | Fase BIBIFI | O que acontece na sala |
| :--- | :--- | :--- |
| **00–05 min** | **Introdução** | Seu grupo explica em 3 minutos o cenário do Controlador Smart Home do artigo (Fulton et al., CCS '22) e entrega a pasta `1_smarthome_esqueleto_colegas/` (`1_smarthome_esqueleto_colegas.ino` + `web.h`) e a folha `2_enunciado_para_os_colegas.md`. |
| **05–20 min** | **1. Build It** *(15 min)* | Os colegas leem o `TODO` no `.ino`, implementam a checagem de permissão (`admin` vs `visitante`) e gravam na NodeMCU. O LED Azul fica aceso. |
| **20–27 min** | **2. Break It (Rodada 1)** *(7 min)* | Seu grupo conecta no Wi-Fi `SmartHome_IoT` dos colegas e roda o **Ataque 1** (pelo script Python `painel_ataque_grupo.py`). O LED Azul apaga! Seu grupo explica as falhas encontradas (*Mistake* e *Some Intuitive No Implementation*). |
| **27–35 min** | **3. Fix It** *(8 min)* | Os colegas corrigem o `return;`, trancam a rota `/api/manutencao` e validam o **Token Hash MD5** (`gerarTokenMD5`). Testam o Ataque 1 e veem que agora dá `403` (bloqueado)! |
| **35–40 min** | **4. Break It (Rodada 2 — Replay Attack)** *(5 min)* | Você pede para os colegas apagarem e acenderem a luz 1 vez usando o comando oficial do Admin (`/admin` no navegador ou `cliente_admin.py`). Seu grupo captura o token na rede e executa a **replicação de comando (Replay Attack)**: o LED Azul **apaga de novo**! |
| **40–45 min** | **5. Fechamento Teórico** *(5 min)* | Seu grupo mostra como isso prova a tese do artigo: desenvolvedores corrigem bugs simples (*Mistakes*), mas deixam brechas de protocolo (*Misunderstanding* / *Unintuitive No Implementation*). Mostra rapidamente a solução com *Nonce* (`3_smarthome_seguro_com_nonce.ino`). |

---

## 3. Como Funcionam as 3 Brechas do Exercício (O "Segredo")

### Brecha 1A (Fase Build): *Mistake* de Fluxo de Controle
- **Base no Artigo:** Seção 3.3 (pág. 1147) — A Equipe T6 trocou o `return` por um `print("FAILED")` durante testes e esqueceu de reverter. O sistema avisava que deu erro, mas executava os comandos mesmo assim.
- **No Código dos Colegas (`1_smarthome_esqueleto_colegas.ino`):**
  Na função `handleControleLuz()`, já existe um `if (user != ADMIN_USER) { registrarAlertaSeguranca(...); } else { // TODO ... }` sem nenhuma dica de bug!
  A função `registrarAlertaSeguranca()` acende o LED Vermelho por 3 segundos e envia o HTTP `403`, **mas não interrompe a função `handleControleLuz()`** se o colega esquecer de colocar `return;` dentro do `if`! E como o código do colega vai ficar dentro do `else`, mesmo que ele coloque `return;` no `else`, quem chamar `/luz?acao=OFF&user=visitante` entra no `if`, pula o `else` e apaga o LED Azul!
- **Como explorar:**
  `http://192.168.4.1/luz?acao=OFF&user=visitante`

### Brecha 1B (Fase Build): *Some Intuitive No Implementation* (Rota Esquecida)
- **Base no Artigo:** Seção 3.1 (pág. 1146) e Seção 5.1 — As Equipes T3, T11 e T12 espalharam verificações de acesso em cada função individual em vez de centralizar. Com a pressa, esqueceram de proteger comandos específicos.
- **No Código dos Colegas (`1_smarthome_esqueleto_colegas.ino`):**
  Eles vão focar os 15 minutos protegendo a função `handleControleLuz()` (rota `/luz`). Porém, lá no final do arquivo existe a rota `/api/manutencao?cmd=apagar_tudo`, criada "para diagnóstico de hardware", que apaga o LED Azul diretamente sem chamar a verificação do `admin`!
- **Importância tática para o seu grupo:** Mesmo que os colegas percebam a Brecha 1A e coloquem o `return;` na rota `/luz`, o seu grupo ainda consegue apagar o LED deles no **Break 1** usando a Brecha 1B (`/api/manutencao?cmd=apagar_tudo`)!

### Brecha 2 (Pós-Fix): *Misunderstanding* + *Unintuitive No Implementation* (Replay Attack)
- **Base no Artigo:** Seções 3.1 e 3.2 — Os alunos tentam proteger a autenticação sem trafegar a senha aberta usando um Token Hash (`MD5("admin:" + SENHA_ADMIN + ":OFF")`), mas não implementam um **Nonce (número de uso único)** porque isso não foi soletrado passo a passo na especificação (*Unintuitive*).
- **O que acontece na Fase *Fix It*:**
  Quando vocês explicarem as Brechas 1A e 1B para os colegas, digam:
  > *"Além de colocar o `return;` e fechar a rota `/api/manutencao`, confiram se o `TODO` está validando corretamente o `token` gerado por `gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF")` para a senha não passar em texto claro!"*
- Os colegas vão atualizar o código (ou você pode carregar direto o `2_smarthome_pos_fix_colegas.ino` para agilizar se o tempo estiver curto).
- **Como o Seu Grupo Executa o Replay Attack (Break 2 com Sniffing Real):**
  1. Inicie o painel com permissão de captura de rede no notebook:
     `sudo python3 ataques/painel_ataque_grupo.py`
  2. Escolha a opção **`[4]`** para ligar o sniffer real (`tcpdump` na interface Wi-Fi, porta `80`).
  3. Peça para um colega (ou você mesmo no navegador do notebook) fazer login em `http://192.168.4.1/admin` (usuário `admin`, senha `123`) e clicar em **Apagar Luz** (ou rodar `python3 ataques/cliente_admin.py OFF` em outro terminal). A requisição que passa pela rede é:
     `GET /luz?acao=OFF&user=admin&token=09b2acb10fcbb3fb37fb442add35bb36`
  4. O sniffer real captura esse pacote HTTP no ar na hora, exibe o token extraído e guarda em memória!
  5. Escolha **`g`** (guardar), use a opção **`[5]`** para acender a luz novamente, e aperte **`[4]` → `Enter`** para disparar o **Replay Attack**.
  6. Pergunte para a turma: *"Qual é a senha secreta do Admin?"* — **Ninguém sabe pela rede**, porque ela está protegida dentro do hash MD5! Mas ao replicar exatamente o comando capturado, a NodeMCU aceita o token e **apaga o LED Azul na hora**!

---

## 4. Comandos Rápidos para Executar no Dia

### Usando os Scripts em Python (No Notebook do Seu Grupo)
1. Conecte o Wi-Fi do notebook na rede `SmartHome_IoT` (senha `12345678`).
2. Para simular o uso legítimo do Admin (acender ou apagar a luz com Token Hash):
   ```bash
   python3 ataques/cliente_admin.py ON
   python3 ataques/cliente_admin.py OFF
   ```
3. Para abrir o painel interativo de ataques do seu grupo (use `sudo` para o sniffer `tcpdump`):
   ```bash
   sudo python3 ataques/painel_ataque_grupo.py
   ```

### Usando o `curl` Direto pelo Terminal (Caso queiram mostrar em 1 linha)
- **Break 1A (Falha de `return` no fluxo de controle):**
  ```bash
  curl "http://192.168.4.1/luz?acao=OFF&user=visitante"
  ```
- **Break 1B (Rota de manutenção esquecida sem controle de acesso):**
  ```bash
  curl "http://192.168.4.1/api/manutencao?cmd=apagar_tudo"
  ```
- **Break 2 (Sniffing real com `tcpdump` + Replay Attack do comando do Admin):**
  ```bash
  # 1. Em um terminal, capture o comando HTTP trafegando na interface Wi-Fi:
  sudo tcpdump -i any -A -nn "tcp port 80 and host 192.168.4.1"
  # 2. Replica (Replay) exatamente o comando capturado com o token do Admin:
  curl "http://192.168.4.1/luz?acao=OFF&user=admin&token=09b2acb10fcbb3fb37fb442add35bb36"
  ```
