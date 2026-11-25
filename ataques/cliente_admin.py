#!/usr/bin/env python3
"""
Cliente Oficial do Administrador - Smart Home IoT (BIBIFI)
Simula o aplicativo legítimo do administrador gerando o Token Hash MD5
(sem trafegar a senha em texto claro na rede) para controlar a luz principal.

Uso:
  python3 ataques/cliente_admin.py OFF
  python3 ataques/cliente_admin.py ON
  python3 ataques/cliente_admin.py OFF --nonce
"""

import hashlib
import json
import sys
import urllib.error
import urllib.request

HOST_PADRAO = "http://192.168.4.1"
USUARIO_ADMIN = "admin"
SENHA_SECRETA_ADMIN = "123"
LARGURA = 64
USA_COR = sys.stdout.isatty()


def cor(texto: str, codigo: str) -> str:
    if not USA_COR:
        return texto
    return f"\033[{codigo}m{texto}\033[0m"


def negrito(t: str) -> str: return cor(t, "1")
def fraco(t: str) -> str:   return cor(t, "2")
def verde(t: str) -> str:   return cor(t, "32")
def amarelo(t: str) -> str: return cor(t, "33")
def ciano(t: str) -> str:   return cor(t, "36")


def linha(char: str = "─") -> str:
    return fraco(char * LARGURA)


def gerar_token_md5(usuario: str, senha: str, acao: str, nonce: str = "") -> str:
    """Gera o mesmo Hash MD5 que a função gerarTokenMD5() da NodeMCU ESP8266."""
    if nonce:
        payload = f"{usuario}:{senha}:{acao}:{nonce}"
    else:
        payload = f"{usuario}:{senha}:{acao}"
    return hashlib.md5(payload.encode("utf-8")).hexdigest()


def requisicao_http(url: str) -> tuple[int, str]:
    try:
        with urllib.request.urlopen(url, timeout=4) as resp:
            return resp.getcode(), resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")
    except Exception as e:
        return 0, str(e)


def executar_comando_admin(acao: str, host: str = HOST_PADRAO, usar_nonce: bool = False) -> None:
    acao = acao.upper()
    nonce = ""

    if usar_nonce:
        code_st, body_st = requisicao_http(f"{host}/status")
        if code_st == 200:
            try:
                dados = json.loads(body_st)
                nonce = dados.get("nonce_atual", "")
            except json.JSONDecodeError:
                pass

    token = gerar_token_md5(USUARIO_ADMIN, SENHA_SECRETA_ADMIN, acao, nonce)
    url = f"{host}/luz?acao={acao}&user={USUARIO_ADMIN}&token={token}"

    print(f"\n{linha('═')}")
    print(f"  {negrito('Cliente Admin')} {fraco('— Smart Home IoT')}")
    print(f"  {fraco('Alvo  :')} {ciano(host)}")
    print(linha('═'))
    print(f"{ciano('▸')} {negrito('Enviando comando autenticado')}")
    print(f"  {fraco('Usuário :')} {USUARIO_ADMIN}")
    print(f"  {fraco('Ação    :')} {acao}")
    if nonce:
        print(f"  {fraco('Nonce   :')} {nonce}")
    print(f"  {fraco('Token   :')} {token}")
    print(f"  {fraco('GET     :')} {url}")

    codigo, resposta = requisicao_http(url)
    print(f"  {fraco('HTTP    :')} [{codigo}] {resposta}")
    print(linha())
    if codigo == 200:
        print(f"  {verde('[Ok]')} Comando executado com sucesso.")
    elif codigo == 0:
        print(f"  {amarelo('[Erro]')} Falha de conexão com o controlador.")
    else:
        print(f"  {amarelo('[Recusado]')} Controlador retornou HTTP {codigo}.")


if __name__ == "__main__":
    acao_arg = sys.argv[1] if len(sys.argv) > 1 else "OFF"
    usar_nonce_flag = "--nonce" in sys.argv
    host_arg = HOST_PADRAO
    for arg in sys.argv[1:]:
        if arg.startswith("http://") or arg.startswith("https://"):
            host_arg = arg
    executar_comando_admin(acao_arg, host=host_arg, usar_nonce=usar_nonce_flag)
