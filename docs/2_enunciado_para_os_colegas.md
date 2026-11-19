# Desafio Prático de Segurança IoT: Controlador Smart Home (BIBIFI)

**Tempo de Desenvolvimento (Fase Build):** 15 a 20 minutos
**Plataforma:** NodeMCU ESP8266 + LEDs (Azul = Luz da Sala | Vermelho = Alarme de Segurança)

---

## 1. Contexto do Problema

Vocês foram contratados para finalizar o firmware de segurança de um **Controlador de Iluminação Smart Home** baseado no ESP8266.
O controlador cria uma rede Wi-Fi própria e possui duas interfaces web:
- **Painel do Convidado (`http://192.168.4.1/`):** Público, permite consultar a iluminação e ligar a luz.
- **Painel do Administrador (`http://192.168.4.1/admin`):** Protegido por tela de login (`admin`), permite ligar e apagar a luz principal.

Por questões de segurança física do prédio, o controlador de comandos (`/luz`) deve aplicar **dois níveis de acesso (RBAC)**:
- **`visitante`**: Pode **LIGAR (`ON`)** a luz da sala, mas **NUNCA pode APAGAR (`OFF`)** a luz.
- **`admin`**: Único perfil autorizado a **APAGAR (`OFF`)** a luz da sala (além de poder ligá-la).

---

## 2. Como Conectar e Testar

1. Abra o arquivo `1_smarthome_esqueleto_colegas.ino` na Arduino IDE e faça o upload para a sua NodeMCU.
2. Abra o **Monitor Serial** em `115200 baud` para acompanhar os logs.
3. Conecte seu notebook ou celular na rede Wi-Fi criada pela placa:
   - **Nome da Rede (SSID):** `SmartHome_IoT` *(ou o nome configurado para a sua equipe)*
   - **Senha do Wi-Fi:** `12345678`
4. Acesse no navegador:
   - Convidado: **`http://192.168.4.1/`**
   - Administrador: **`http://192.168.4.1/admin`** *(usuário `admin`, senha `seguranca2026`)*

---

## 3. Requisitos de Segurança (Sua Missão na Fase *Build It*)

A interface web já está separada, mas a rota de controle do hardware (`/luz`, função `handleControleLuz`) ainda precisa ter a autenticação do administrador concluída. Vocês têm **15 minutos** para completar o bloco `// TODO` garantindo que:

### Requisito 1 — Controle de Acesso por Papel (RBAC)
- Qualquer perfil (`visitante` ou `admin`) pode enviar `acao=ON` para acender a luz.
- Apenas o usuário `admin` autenticado pode executar `acao=OFF` para apagar a luz.
- Tentativas não autorizadas devem acionar `registrarAlertaSeguranca(...)` (que acende o LED Vermelho de alarme por 3 segundos e retorna HTTP `403`).

### Requisito 2 — Autenticação do Administrador no Comando `OFF`
- Quando `acao == "OFF"`, valide a credencial do administrador:
  - Aceite a `senha` igual a `SENHA_ADMIN` **OU** o `token` correspondente ao hash gerado por `gerarTokenMD5(ADMIN_USER, SENHA_ADMIN, "OFF")`.
- **Importante para evitar senha aberta na rede:** O painel `/admin` envia o comando usando o **Token Hash MD5** no parâmetro `token` da URL para não expor a senha em texto claro.

---

## 4. Regras da Dinâmica (Build It, Break It, Fix It)

1. **Fase 1 — Build It (15 min):** Implementem a segurança no código e gravem na NodeMCU. A Luz Principal (LED Azul) deve começar **ACESA**.
2. **Fase 2 — Break It (7 min):** A equipe atacante vai se conectar na rede Wi-Fi da placa de vocês e tentar **apagar o LED Azul** sem conhecer a senha do `admin`.
3. **Fase 3 — Fix It (8 min):** Caso a equipe atacante consiga apagar a luz de vocês, eles explicarão a brecha encontrada. Vocês terão 8 minutos para corrigir o código e gravar novamente na placa para o **Desafio Final**.
