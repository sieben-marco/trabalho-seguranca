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

## 3. Como a Nossa Atividade de 30–45 Minutos Reproduz o Artigo

Adaptamos o projeto do artigo (**Controlador IoT de Casa Inteligente**) para rodar em **30 a 45 minutos** utilizando **NodeMCU ESP8266 + LEDs** em rede própria (`SoftAP`).

| Elemento do Artigo (Fulton et al., CCS '22) | Equivalente na Nossa Atividade Prática (30–45 min) |
| :--- | :--- |
| **Controlador IoT Smart Home** | NodeMCU ESP8266 controlando a **Luz Principal da Sala (LED Azul)** e o **Alerta de Segurança (LED Vermelho)**. |
| **Controle de Acesso Baseado em Papéis (RBAC)** | Dois papéis: `admin` (pode ligar e apagar a luz) e `visitante` (só pode consultar status e ligar a luz, nunca apagar). |
| **Fase 1: Build It (15 min)** | Os colegas recebem o código-esqueleto funcional e implementam as regras de autenticação e autorização na função da luz. |
| **Vulnerabilidade 1A: *Mistake* (Fluxo de Controle)** | Inspirada literalmente na **Equipe T6 do artigo**: a função `registrarAlertaSeguranca()` acende o LED Vermelho e imprime `"ACESSO NEGADO"`, mas se o aluno não colocar `return;` logo depois, a função segue em frente e apaga o LED Azul! |
| **Vulnerabilidade 1B: *Some Intuitive No Implementation*** | Inspirada nas **Equipes T3 e T11 do artigo**: os colegas colocam a verificação de senha na rota `/luz`, mas esquecem a rota secundária `/api/manutencao` no final do arquivo. |
| **Fase 2: Break It — Rodada 1 (7 min)** | Seu grupo explora o *Mistake* ou a rota esquecida, apaga o LED Azul sem senha e explica a falha para os colegas. |
| **Fase 3: Fix It (8 min)** | Os colegas corrigem o `return;`, trancam a rota secundária e implementam a autenticação por **Token Hash (`MD5(usuario:senha:acao)`)** para não enviar a senha em texto claro no Wi-Fi. |
| **Fase 4: Break It — Rodada 2: *Replay Attack* (5 min)** | **A grande sacada do exercício:** Como o requisito de uso único (*Nonce*) é *Unintuitive* (não explícito na especificação) e há um *Misunderstanding* sobre hashes estáticos, o seu grupo captura o comando legítimo do Admin e **replica o pacote (Replay Attack)**, apagando o LED Azul novamente mesmo sem saber a senha! |
| **Fase 5: Debriefing e Solução com *Nonce* (5 min)** | Demonstração do código `3_smarthome_seguro_com_nonce.ino`, mostrando que sem segurança no design do protocolo (*Challenge-Response* / *Nonce*), corrigir apenas bugs superficiais não impede ataques de repetição. |

---

## 4. Paralelo Técnico: Servidor Web HTTP vs. Protocolo ESP-NOW

O professor sugeriu explorar o **ESP-NOW** ou um **Servidor Web**. Optamos pelo **Servidor Web HTTP em Rede Própria (`SoftAP`)** para garantir que os colegas consigam programar e testar em 15 minutos, mas **o fenômeno de segurança demonstrado no ataque final é idêntico ao do ESP-NOW**:

1. **Como seria no ESP-NOW:**
   - O ESP-NOW transmite pequenos pacotes binários (`struct_message`) diretamente na camada de enlace (quadros de ação IEEE 802.11) entre o endereço MAC de duas placas ESP8266.
   - Mesmo quando os alunos colocam um campo `char token[33]` com um hash MD5/SHA-256 dentro do `struct` do ESP-NOW para autenticar o comando `"APAGAR_LUZ"`, qualquer outra placa ESP8266 no mesmo canal Wi-Fi (ou em modo promíscuo/broadcast) pode escutar os bytes desse pacote no ar e retransmiti-los (`esp_now_send`).
   - Sem um contador monotônico de pacotes ou um desafio aleatório (*Nonce*), o receptor ESP-NOW aceita o pacote clonado e apaga o LED.

2. **Por que o Servidor Web HTTP em Rede Própria é didaticamente superior para 30–45 min:**
   - **Visibilidade em sala:** Os pacotes, parâmetros (`user=admin&acao=OFF&token=...`) e códigos de resposta (`200 OK` vs `403 Proibido`) podem ser vistos no navegador, no terminal Python ou no Monitor Serial por toda a turma.
   - **Agilidade no *Build* e no *Fix*:** No ESP-NOW, cada equipe precisaria editar e gravar duas placas a cada alteração (Transmissor + Receptor). Com o Servidor Web, os colegas gravam apenas o Controlador Smart Home e testam imediatamente pelo celular/notebook, enquanto o seu grupo pode atacar tanto pelo notebook quanto usando uma **segunda NodeMCU programada como "Placa Atacante" (`4_placa_atacante_redteam.ino`)**!
