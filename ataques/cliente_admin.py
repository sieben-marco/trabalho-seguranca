#!/usr/bin/env python3
"""
Cliente Oficial do Administrador - Smart Home IoT (BIBIFI)
Simula o aplicativo legítimo do Admin que gera o Token Hash MD5 (sem enviar a
senha em texto claro na rede) para controlar a Luz Principal na NodeMCU.

Uso:
  python3 ataques/cliente_admin.py OFF
  python3 ataques/cliente_admin.py ON
"""

import hashlib
import json
import sys
import urllib.error
import urllib.parse
import urllib.request

HOST_PADRAO = "http://192.168.4.1"
USUARIO_ADMIN = "admin"
SENHA_SECRETA_ADMIN = "seguranca2026"


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

    print("=" * 68)
    print("📱 [APP OFICIAL DO ADMIN] Enviando comando autenticado...")
    print(f"   • Usuário: {USUARIO_ADMIN}")
    print(f"   • Ação:    {acao}")
    if nonce:
        print(f"   • Nonce:   {nonce}")
    print(f"   • Token MD5 calculado: {token}")
    print(f"   • Requisição enviada:  GET {url}")
    print("-" * 68)

    codigo, resposta = requisicao_http(url)
    print(f"   • Resposta HTTP [{codigo}]: {resposta}")
    print("=" * 68)


if __name__ == "__main__":
    acao_arg = sys.argv[1] if len(sys.argv) > 1 else "OFF"
    usar_nonce_flag = "--nonce" in sys.argv
    host_arg = HOST_PADRAO
    for arg in sys.argv[1:]:
        if arg.startswith("http://") or arg.startswith("https://"):
            host_arg = arg
    executar_comando_admin(acao_arg, host=host_arg, usar_nonce=usar_nonce_flag)
