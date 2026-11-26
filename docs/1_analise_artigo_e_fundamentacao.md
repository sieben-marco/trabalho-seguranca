# Análise do Artigo e Fundamentação da Atividade Prática

**Artigo Base:** *Understanding the How and the Why: Exploring Secure Development Practices through a Course Competition*
**Autores:** Kelsey R. Fulton, Daniel Votipka, Desiree Abrokwa, Michelle L. Mazurek, Michael Hicks e James Parker (ACM CCS 2022).
**Arquivo Original:** [Understanding the How and the Why - Exploring Secure Development Practices through a Course Competition.pdf](Understanding%20the%20How%20and%20the%20Why%20-%20Exploring%20Secure%20Development%20Practices%20through%20a%20Course%20Competition.pdf)

---

## 1. Resumo Executivo do Artigo

O artigo investiga **como e por que** desenvolvedores introduzem vulnerabilidades ao programar sistemas com requisitos de segurança, como outras equipes encontram essas brechas (revisão/ataque) e por que certas falhas são corrigidas enquanto outras permanecem abertas.

Para estudar isso de forma realista, os autores acompanharam **14 equipes** de estudantes de graduação ao longo de uma competição no formato **BIBIFI (*Build It, Break It, Fix It*)**, dividida em três fases:
1. **Build It (Construir):** As equipes desenvolvem um **Controlador IoT de Casa Inteligente (*Smart Home Controller*)** seguindo uma especificação funcional e de segurança (controle de luzes, termostato, sensores, autenticação por senha e controle de acesso baseado em papéis — *RBAC*).
2. **Break It (Quebrar):** O código das equipes é aberto para que os colegas encontrem vulnerabilidades e submetam *exploits* (casos de teste que comprovam a falha).
3. **Fix It (Corrigir):** Simultaneamente aos ataques, os desenvolvedores tentam corrigir as vulnerabilidades exploradas em seu código para estancar a perda de pontos.

---

## 2. Principais Descobertas das 3 Questões de Pesquisa (RQs)

### RQ1: Quais tipos de vulnerabilidades os desenvolvedores introduzem e por quê?
Os autores identificaram **145 vulnerabilidades únicas** durante a fase *Build* (restaram 79 ao final dessa fase) e as classificaram em **3 grandes categorias** (Seção 3 e Tabela 1 do artigo):

1. **No Implementation (Falta de Implementação — 72 introduzidas / 29 restantes pós-Build):**
   Ocorre quando a equipe simplesmente deixa de implementar um mecanismo de segurança necessário. Divide-se em três subtipos:
   - *All Intuitive:* Requisitos explícitos e óbvios que foram totalmente esquecidos (ex.: esquecer de implementar *timeout* de 30s ou controle de acesso geral por falta de tempo).
   - *Some Intuitive:* A equipe implementou controle de acesso na maioria dos comandos, mas **esqueceu em algum comando específico** (ex.: Equipes T3 e T11, que colocaram checagens individuais em cada função em vez de centralizar em uma única função — violando o princípio de *código mínimo confiável*).
   - *Unintuitive:* Requisitos de segurança **implícitos** que não estavam descritos como uma receita de bolo na especificação (no artigo, a checagem recursiva de delegação de permissões; no nosso exercício IoT, a **proteção contra Replicação de Comando / Replay Attack**).

2. **Misunderstanding (Entendimento Incorreto — 34 introduzidas / 25 restantes pós-Build):**
   A equipe **tentou** implementar a segurança exigida, mas compreendeu errado o conceito de segurança ou as premissas do sistema.
   - Exemplo do artigo: Achar que revogar uma permissão ou validar apenas o usuário logado principal bastava, ou assumir que as entradas do usuário sempre seguiriam o formato benigno dos testes fornecidos.
   - Na nossa atividade: Achar que substituir a senha em texto claro por um **Token/Hash estático** (`MD5(usuario + senha + comando)`) impede que um atacante na mesma rede apague a luz, sem perceber que o próprio hash passa a funcionar como uma "senha reutilizável" se for capturado e replicado (*Replay Attack*).

3. **Mistake (Erro de Programação — 39 introduzidas / 25 restantes pós-Build):**
   A equipe entendeu o requisito e tentou implementá-lo, mas cometeu um bug lógico ou de fluxo de controle (*control flow*).
   - **Caso clássico da Equipe T6 no artigo (Seção 3.3, pág. 1147):** Ao detectar uma falha ou violação de segurança, o sistema deveria abortar e reverter as alterações retornando `"FAILED"`. Porém, para facilitar a depuração (*debug*), a equipe trocou o `return` por um `print("FAILED")` e esqueceu de reverter! O código avisava que falhou, mas **continuava executando o comando proibido**.

#### Impacto do Cronograma e do Design (Seção 3.4)
- **Deixar segurança para o final é fatal:** Equipes que focaram apenas em funcionalidade no começo e deixaram segurança para os últimos dias (como a Equipe T11) ficaram sem tempo e introduziram dezenas de falhas *No Implementation* e *Mistake*.
- **Design detalhado evita omissões, mas cristaliza erros conceituais:** Equipes com bom design inicial quase não tiveram falhas *No Implementation*, mas quando tinham um *Misunderstanding* conceitual no design inicial, mantiveram esse erro até o fim sem perceber.

---

### RQ2: Como as vulnerabilidades são descobertas na revisão/ataque? (Seção 4)
- **Erros simples (*Mistakes* e *Some Intuitive No Implementations*) são encontrados quase por acidente:** 22 das 25 falhas de *Mistake* foram exploradas na fase *Break*. Basta testar funcionalidades básicas ou entradas malformadas para esbarrar nelas.
- **Equipes atacantes preferem alvos fáceis:** Quando o código tem problemas óbvios (como falta de checagem em uma rota ou erro de fluxo), os atacantes focam neles primeiro.
- **Falhas de *Misunderstanding* exigem testes direcionados (*targeted testing*):** Só são exploradas por quem analisa a lógica do protocolo/controle de acesso em busca de brechas conceituais.

---

### RQ3: Como e por que os desenvolvedores corrigem (ou não) as falhas? (Seção 5)
- **Mistakes são fáceis de corrigir:** Uma vez apontados, geralmente exigem mudar 1 ou 2 linhas de código (ex.: adicionar um `return` ou validação de nulo).
- **Misunderstandings só são corrigidos quando explicados pelo atacante (Seção 5.2):** Os desenvolvedores não percebiam que sua lógica de segurança estava falha até sofrerem o ataque na fase *Break* e lerem a explicação.
- **Requisitos implícitos (*Unintuitive*) e falhas arquiteturais ficam sem correção sob pressão de tempo:** Mais da metade das falhas *No Implementation* restantes não foram corrigidas até o final do estudo porque corrigi-las exigiria reestruturar o sistema de autenticação/autorização (como a Equipe T12), e o tempo da atividade acabou.

---

## 3. Como a Nossa Demonstração de ~10 Minutos Reproduz o Artigo

Adaptamos o cenário do artigo (**Controlador IoT de Casa Inteligente**) para **3 placas Seeed Studio XIAO ESP32-C6** comunicando por **ESP-NOW** em **4 etapas progressivas**, mantendo uma interface web (`web.h`) no Hub (`XIAO #2`) para o controle por celular/navegador e um Sniffer em modo monitor (`XIAO #3`) ouvindo o rádio no ar:

| Elemento do Artigo (Fulton et al., CCS '22) | Equivalente na Nossa Demonstração (4 Etapas com XIAO ESP32-C6) |
| :--- | :--- |
| **Controlador IoT Smart Home** | **XIAO #1 (Casa)** controlando **Luz Principal (LED Azul `D10`)** e **Alarme Intermitente (LED Vermelho `D9`, 700 ms)** multiplexados com 1 resistor + **XIAO #2 (Hub Web)**. |
| **Controle de Acesso Baseado em Papéis (RBAC)** | Dois papéis no Hub Web: `admin` (pode ligar, apagar e silenciar o alarme) e `visitante` (só pode consultar status e ligar a luz, nunca apagar). |
| **Etapa 1: *Mistake* (Fluxo de Controle)** | Inspirada literalmente na **Equipe T6 do artigo**: ao negar `OFF` para `visitante` no Hub (`etapa1_hub_web`), o sistema dispara o alarme vermelho e retorna HTTP `403`, mas por falta de `return;` envia o comando `OFF` pelo rádio ESP-NOW e apaga o LED Azul! Além disso, o tráfego ESP-NOW trafega em Broadcast e texto claro. |
| **Etapa 2: *Misunderstanding* (AES sem Anti-Replay)** | Corrigido o `return;` e adicionada criptografia **AES-128** em **Unicast** (`etapa2_*`). O Sniffer (`XIAO #3`) não consegue mais decifrar o comando vindo do celular (`16B ?`), mas faz **Replay verbatim** dos bytes cifrados e apaga o LED Azul — provando que confidencialidade não garante autenticidade nem proteção contra repetição. |
| **Etapa 3: *Unintuitive* / Estado Volátil (Contador em RAM)** | Adiciona-se um **contador sequencial (`seq > ultimoSeq`)** dentro do bloco AES (`etapa3_*`). O Replay normal é bloqueado (disparando o alarme), mas ao **reiniciar / cortar a energia da XIAO #1**, `ultimoSeq` volta a `0` e o mesmo pacote antigo volta a ser aceito (*Reset-to-Zero Replay*). |
| **Etapa 4: Defesa Completa (Nonce Aleatório)** | Implementa-se **Desafio-Resposta com Nonce de 32 bits gerado por hardware (`esp_random()`)** e validade de 3 segundos (`etapa4_*`). Bloqueia Replay normal e permanece seguro mesmo após reinicialização forçada, disparando o LED Vermelho piscante até novo login válido do administrador. |

---

## 4. Arquitetura Híbrida: Hub Web HTTP + Enlace Sem Fio ESP-NOW

O professor sugeriu explorar o **ESP-NOW** com uma placa em modo Sniffer. Em vez de escolher entre "só Servidor Web" ou "só ESP-NOW sem interface", combinamos ambos em **2 saltos** (como em hubs IoT reais, ex.: Zigbee/Matter):

1. **Salto 1 — Celular/Navegador ↔ XIAO #2 (Hub Web via Wi-Fi SoftAP no Canal 1):**
   - Mantém a interface visual amigável (`web.h`) para demonstrar RBAC (`Convidado` vs. `Administrador`) e exibir em tempo real quando um ataque altera o estado da lâmpada.
2. **Salto 2 — XIAO #2 (Hub Web) ↔ XIAO #1 (Casa/LEDs via ESP-NOW no Canal 1):**
   - O comando de atuação viaja pelo ar em quadros 802.11 Action do protocolo **ESP-NOW**.
   - A **XIAO #3 (Sniffer em modo promíscuo)** escuta o Canal 1 independentemente do Wi-Fi do celular (contornando a proteção anti-replay da camada WPA2/TCP) e demonstra **ao vivo** tanto a captura em texto claro quanto o **Replay Attack verbatim** sobre pacotes **AES-128** criptografados — seguido da quebra do contador em RAM via reset e da defesa definitiva com **Nonce aleatório**.
