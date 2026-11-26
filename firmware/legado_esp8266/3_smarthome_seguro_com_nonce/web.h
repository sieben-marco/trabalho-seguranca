#pragma once

// ============================================================================
//  Interface Web (versão com Nonce anti-replay) — Flash (PROGMEM)
//  Estilo Neumorphism (Dark / Light / Sistema) sem emojis e sem recarregar
// ============================================================================

const char CONVIDADO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Home — Convidado</title>
  <script>
    (function(){
      var t = localStorage.getItem('sh_theme') || 'auto';
      if (t === 'light' || t === 'dark') document.documentElement.setAttribute('data-theme', t);
    })();
  </script>
  <style>
    :root {
      color-scheme: light dark;
      --bg: #e4e9f2;
      --sh-d: #c4cbd8;
      --sh-l: #ffffff;
      --btn-bg: linear-gradient(145deg, #eef2f9, #d5dce8);
      --pri-bg: linear-gradient(145deg, #2b6ef2, #1d4ed8);
      --dan-bg: linear-gradient(145deg, #e53935, #b91c1c);
      --text: #1e293b;
      --muted: #64748b;
      --accent: #2563eb;
      --accent-glow: rgba(37, 99, 235, 0.35);
      --danger: #dc2626;
      --ok: #16a34a;
      --r: 20px;
    }
    @media (prefers-color-scheme: dark) {
      :root:not([data-theme="light"]) {
        --bg: #1c2028;
        --sh-d: #12151b;
        --sh-l: #262b36;
        --btn-bg: linear-gradient(145deg, #252a35, #181c23);
        --pri-bg: linear-gradient(145deg, #3b82f6, #2563eb);
        --dan-bg: linear-gradient(145deg, #ef4444, #dc2626);
        --text: #e5e7eb;
        --muted: #94a3b8;
        --accent: #3b82f6;
        --accent-glow: rgba(59, 130, 246, 0.45);
        --danger: #ef4444;
        --ok: #22c55e;
      }
    }
    :root[data-theme="dark"] {
      --bg: #1c2028;
      --sh-d: #12151b;
      --sh-l: #262b36;
      --btn-bg: linear-gradient(145deg, #252a35, #181c23);
      --pri-bg: linear-gradient(145deg, #3b82f6, #2563eb);
      --dan-bg: linear-gradient(145deg, #ef4444, #dc2626);
      --text: #e5e7eb;
      --muted: #94a3b8;
      --accent: #3b82f6;
      --accent-glow: rgba(59, 130, 246, 0.45);
      --danger: #ef4444;
      --ok: #22c55e;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      padding: 24px 16px;
      transition: background 0.2s, color 0.2s;
    }
    .wrap {
      width: 100%;
      max-width: 440px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    .topbar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      flex-wrap: wrap;
    }
    .brand h1 { font-size: 1.15rem; letter-spacing: 0.04em; font-weight: 700; }
    .brand span {
      font-size: 0.72rem;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.1em;
    }
    .theme-seg {
      display: inline-flex;
      padding: 4px;
      border-radius: 999px;
      background: var(--bg);
      box-shadow: inset 3px 3px 6px var(--sh-d), inset -3px -3px 6px var(--sh-l);
      gap: 2px;
    }
    .theme-seg button {
      border: none;
      background: transparent;
      color: var(--muted);
      font-size: 0.7rem;
      font-weight: 600;
      padding: 6px 10px;
      border-radius: 999px;
      cursor: pointer;
      transition: all 0.15s;
    }
    .theme-seg button.active {
      color: var(--accent);
      background: var(--bg);
      box-shadow: 3px 3px 6px var(--sh-d), -3px -3px 6px var(--sh-l);
    }
    .card {
      background: var(--bg);
      border-radius: var(--r);
      padding: 22px;
      box-shadow: 9px 9px 18px var(--sh-d), -9px -9px 18px var(--sh-l);
    }
    .card h2 {
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.12em;
      color: var(--muted);
      margin-bottom: 16px;
    }
    .status-row {
      display: flex;
      align-items: center;
      gap: 18px;
    }
    .led-socket {
      width: 58px;
      height: 58px;
      border-radius: 50%;
      flex-shrink: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
    }
    .led-core {
      width: 28px;
      height: 28px;
      border-radius: 50%;
      background: var(--muted);
      opacity: 0.3;
      transition: all 0.25s;
    }
    .led-core.on {
      background: var(--accent);
      opacity: 1;
      box-shadow: 0 0 18px var(--accent-glow), 0 0 6px var(--accent);
    }
    .status-text strong { display: block; font-size: 1.1rem; letter-spacing: 0.02em; }
    .status-text small { font-size: 0.78rem; color: var(--muted); }
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
      padding: 13px 16px;
      border: none;
      border-radius: 14px;
      background: var(--btn-bg);
      color: var(--text);
      font-size: 0.86rem;
      font-weight: 600;
      text-decoration: none;
      cursor: pointer;
      box-shadow: 6px 6px 12px var(--sh-d), -6px -6px 12px var(--sh-l);
      transition: box-shadow 0.14s, opacity 0.15s, transform 0.1s;
    }
    .btn:active:not(:disabled) {
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
      transform: scale(0.99);
    }
    .btn-primary { background: var(--pri-bg); color: #ffffff; }
    .btn-primary:active:not(:disabled) {
      box-shadow: inset 3px 3px 7px rgba(0,0,0,0.3), inset -3px -3px 7px rgba(255,255,255,0.16);
    }
    .btn-muted { background: var(--btn-bg); color: var(--text); }
    .btn:disabled {
      background: var(--bg);
      color: var(--muted);
      box-shadow: inset 3px 3px 6px var(--sh-d), inset -3px -3px 6px var(--sh-l);
      opacity: 0.6;
      cursor: not-allowed;
      transform: none;
    }
    .toast {
      position: fixed;
      left: 50%;
      bottom: 24px;
      transform: translate(-50%, 16px);
      max-width: calc(100vw - 32px);
      display: inline-flex;
      align-items: center;
      gap: 10px;
      padding: 12px 18px;
      border-radius: 14px;
      background: var(--bg);
      color: var(--text);
      font-size: 0.84rem;
      font-weight: 600;
      box-shadow: 8px 8px 16px var(--sh-d), -8px -8px 16px var(--sh-l);
      opacity: 0;
      pointer-events: none;
      transition: opacity 0.2s, transform 0.22s;
      z-index: 50;
    }
    .toast.show { opacity: 1; transform: translate(-50%, 0); }
    .toast-dot { width: 9px; height: 9px; border-radius: 50%; flex-shrink: 0; }
    .toast.ok .toast-dot  { background: var(--ok); box-shadow: 0 0 8px var(--ok); }
    .toast.err .toast-dot { background: var(--danger); box-shadow: 0 0 8px var(--danger); }
    .desc { font-size: 0.82rem; color: var(--muted); line-height: 1.45; }
  </style>
</head>
<body>
  <main class="wrap">
    <header class="topbar">
      <div class="brand">
        <h1>Smart Home IoT</h1>
        <span>Área do Convidado</span>
      </div>
      <div class="theme-seg" role="group" aria-label="Tema da interface">
        <button type="button" data-t="auto">Auto</button>
        <button type="button" data-t="light">Claro</button>
        <button type="button" data-t="dark">Escuro</button>
      </div>
    </header>

    <section class="card">
      <h2>Iluminação Principal</h2>
      <div class="status-row">
        <div class="led-socket"><div class="led-core" id="led"></div></div>
        <div class="status-text">
          <strong id="estado">Carregando...</strong>
          <small id="conn">Sincronizando com o controlador</small>
        </div>
      </div>
    </section>

    <section class="card">
      <h2>Controles de Visitante</h2>
      <p class="desc">Convidados podem ligar a luz principal, mas não possuem permissão para apagá-la.</p>
      <div style="margin-top:14px">
        <button type="button" class="btn btn-primary" id="btnOn">Ligar Luz</button>
      </div>
    </section>

    <nav>
      <a class="btn btn-muted" href="/admin">Acesso Administrador</a>
    </nav>
  </main>

  <div class="toast" id="toast" role="status" aria-live="polite">
    <span class="toast-dot"></span>
    <span id="toastTxt"></span>
  </div>

  <script>
    const led = document.getElementById('led');
    const estado = document.getElementById('estado');
    const conn = document.getElementById('conn');
    const toast = document.getElementById('toast');
    const toastTxt = document.getElementById('toastTxt');
    const btnOn = document.getElementById('btnOn');
    let ocupado = false;
    let luzAcesaAtual = null;
    let toastTimer = null;

    function sincronizarBotoes() {
      btnOn.disabled = ocupado || luzAcesaAtual === true;
    }

    function aplicarTema(mode) {
      localStorage.setItem('sh_theme', mode);
      if (mode === 'light' || mode === 'dark') {
        document.documentElement.setAttribute('data-theme', mode);
      } else {
        document.documentElement.removeAttribute('data-theme');
      }
      document.querySelectorAll('.theme-seg button').forEach(b => {
        b.classList.toggle('active', b.dataset.t === mode);
      });
    }
    document.querySelectorAll('.theme-seg button').forEach(b => {
      b.addEventListener('click', () => aplicarTema(b.dataset.t));
    });
    aplicarTema(localStorage.getItem('sh_theme') || 'auto');

    function mostrarToast(ok, texto) {
      toastTxt.textContent = texto;
      toast.className = 'toast show ' + (ok ? 'ok' : 'err');
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => toast.classList.remove('show'), 3200);
    }

    async function atualizarStatus() {
      if (ocupado) return;
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 1400);
      try {
        const r = await fetch('/status', { signal: c.signal, cache: 'no-store' });
        const d = await r.json();
        luzAcesaAtual = !!d.luz_acesa;
        led.classList.toggle('on', luzAcesaAtual);
        estado.textContent = luzAcesaAtual ? 'Luz acesa' : 'Luz apagada';
        conn.textContent = 'Controlador online (192.168.4.1)';
        sincronizarBotoes();
      } catch (_) {
        conn.textContent = 'Sem resposta do controlador';
      } finally {
        clearTimeout(t);
      }
    }

    async function enviarComando(acao) {
      if (ocupado) return;
      ocupado = true;
      btnOn.disabled = true;
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 2000);
      try {
        const r = await fetch('/luz?acao=' + acao + '&user=visitante', { signal: c.signal, cache: 'no-store' });
        if (r.ok) {
          mostrarToast(true, acao === 'ON' ? 'Luz acesa com sucesso' : 'Luz apagada com sucesso');
        } else if (r.status === 403) {
          mostrarToast(false, 'Sem permissão para esta ação');
        } else {
          mostrarToast(false, 'Erro inesperado ao comunicar com o controlador');
        }
      } catch (_) {
        mostrarToast(false, 'Não foi possível conectar ao controlador');
      } finally {
        clearTimeout(t);
        ocupado = false;
        sincronizarBotoes();
        atualizarStatus();
      }
    }

    btnOn.addEventListener('click', () => enviarComando('ON'));

    atualizarStatus();
    setInterval(atualizarStatus, 2000);
  </script>
</body>
</html>
)rawliteral";

const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Home — Admin Anti-Replay</title>
  <script>
    (function(){
      var t = localStorage.getItem('sh_theme') || 'auto';
      if (t === 'light' || t === 'dark') document.documentElement.setAttribute('data-theme', t);
    })();
  </script>
  <style>
    :root {
      color-scheme: light dark;
      --bg: #e4e9f2;
      --sh-d: #c4cbd8;
      --sh-l: #ffffff;
      --btn-bg: linear-gradient(145deg, #eef2f9, #d5dce8);
      --pri-bg: linear-gradient(145deg, #2b6ef2, #1d4ed8);
      --dan-bg: linear-gradient(145deg, #e53935, #b91c1c);
      --text: #1e293b;
      --muted: #64748b;
      --accent: #2563eb;
      --accent-glow: rgba(37, 99, 235, 0.35);
      --danger: #dc2626;
      --ok: #16a34a;
      --r: 20px;
    }
    @media (prefers-color-scheme: dark) {
      :root:not([data-theme="light"]) {
        --bg: #1c2028;
        --sh-d: #12151b;
        --sh-l: #262b36;
        --btn-bg: linear-gradient(145deg, #252a35, #181c23);
        --pri-bg: linear-gradient(145deg, #3b82f6, #2563eb);
        --dan-bg: linear-gradient(145deg, #ef4444, #dc2626);
        --text: #e5e7eb;
        --muted: #94a3b8;
        --accent: #3b82f6;
        --accent-glow: rgba(59, 130, 246, 0.45);
        --danger: #ef4444;
        --ok: #22c55e;
      }
    }
    :root[data-theme="dark"] {
      --bg: #1c2028;
      --sh-d: #12151b;
      --sh-l: #262b36;
      --btn-bg: linear-gradient(145deg, #252a35, #181c23);
      --pri-bg: linear-gradient(145deg, #3b82f6, #2563eb);
      --dan-bg: linear-gradient(145deg, #ef4444, #dc2626);
      --text: #e5e7eb;
      --muted: #94a3b8;
      --accent: #3b82f6;
      --accent-glow: rgba(59, 130, 246, 0.45);
      --danger: #ef4444;
      --ok: #22c55e;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      padding: 24px 16px;
      transition: background 0.2s, color 0.2s;
    }
    .wrap {
      width: 100%;
      max-width: 440px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    .topbar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      flex-wrap: wrap;
    }
    .brand h1 { font-size: 1.15rem; letter-spacing: 0.04em; font-weight: 700; }
    .brand span {
      font-size: 0.72rem;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.1em;
    }
    .theme-seg {
      display: inline-flex;
      padding: 4px;
      border-radius: 999px;
      background: var(--bg);
      box-shadow: inset 3px 3px 6px var(--sh-d), inset -3px -3px 6px var(--sh-l);
      gap: 2px;
    }
    .theme-seg button {
      border: none;
      background: transparent;
      color: var(--muted);
      font-size: 0.7rem;
      font-weight: 600;
      padding: 6px 10px;
      border-radius: 999px;
      cursor: pointer;
      transition: all 0.15s;
    }
    .theme-seg button.active {
      color: var(--accent);
      background: var(--bg);
      box-shadow: 3px 3px 6px var(--sh-d), -3px -3px 6px var(--sh-l);
    }
    .card {
      background: var(--bg);
      border-radius: var(--r);
      padding: 22px;
      box-shadow: 9px 9px 18px var(--sh-d), -9px -9px 18px var(--sh-l);
    }
    .card h2 {
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.12em;
      color: var(--muted);
      margin-bottom: 16px;
    }
    .status-row {
      display: flex;
      align-items: center;
      gap: 16px;
    }
    .led-socket {
      width: 48px;
      height: 48px;
      border-radius: 50%;
      flex-shrink: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
    }
    .led-core {
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: var(--muted);
      opacity: 0.3;
      transition: all 0.25s;
    }
    .led-core.on {
      background: var(--accent);
      opacity: 1;
      box-shadow: 0 0 16px var(--accent-glow), 0 0 5px var(--accent);
    }
    .status-text strong { display: block; font-size: 1rem; }
    .status-text small { font-size: 0.76rem; color: var(--muted); }
    label {
      display: block;
      font-size: 0.76rem;
      font-weight: 600;
      color: var(--muted);
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 0.06em;
    }
    input {
      width: 100%;
      padding: 12px 14px;
      margin-bottom: 14px;
      border: none;
      border-radius: 12px;
      background: var(--bg);
      color: var(--text);
      font-size: 0.9rem;
      outline: none;
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
    }
    input:focus { outline: 2px solid var(--accent); outline-offset: -2px; }
    .well {
      border-radius: 12px;
      padding: 10px 14px;
      font-size: 0.78rem;
      color: var(--muted);
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
      word-break: break-word;
    }
    .well b { color: var(--text); }
    .mono { font-family: ui-monospace, SFMono-Regular, Consolas, monospace; color: var(--accent); }
    .btn-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 14px;
      margin-top: 14px;
    }
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
      padding: 13px 16px;
      border: none;
      border-radius: 14px;
      background: var(--btn-bg);
      color: var(--text);
      font-size: 0.86rem;
      font-weight: 600;
      text-decoration: none;
      cursor: pointer;
      box-shadow: 6px 6px 12px var(--sh-d), -6px -6px 12px var(--sh-l);
      transition: box-shadow 0.14s, opacity 0.15s, transform 0.1s;
    }
    .btn:active:not(:disabled) {
      box-shadow: inset 4px 4px 8px var(--sh-d), inset -4px -4px 8px var(--sh-l);
      transform: scale(0.99);
    }
    .btn-primary { background: var(--pri-bg); color: #ffffff; }
    .btn-danger  { background: var(--dan-bg); color: #ffffff; }
    .btn-primary:active:not(:disabled), .btn-danger:active:not(:disabled) {
      box-shadow: inset 3px 3px 7px rgba(0,0,0,0.3), inset -3px -3px 7px rgba(255,255,255,0.16);
    }
    .btn-muted { background: var(--btn-bg); color: var(--text); }
    .btn:disabled {
      background: var(--bg);
      color: var(--muted);
      box-shadow: inset 3px 3px 6px var(--sh-d), inset -3px -3px 6px var(--sh-l);
      opacity: 0.6;
      cursor: not-allowed;
      transform: none;
    }
    .toast {
      position: fixed;
      left: 50%;
      bottom: 24px;
      transform: translate(-50%, 16px);
      max-width: calc(100vw - 32px);
      display: inline-flex;
      align-items: center;
      gap: 10px;
      padding: 12px 18px;
      border-radius: 14px;
      background: var(--bg);
      color: var(--text);
      font-size: 0.84rem;
      font-weight: 600;
      box-shadow: 8px 8px 16px var(--sh-d), -8px -8px 16px var(--sh-l);
      opacity: 0;
      pointer-events: none;
      transition: opacity 0.2s, transform 0.22s;
      z-index: 50;
    }
    .toast.show { opacity: 1; transform: translate(-50%, 0); }
    .toast-dot { width: 9px; height: 9px; border-radius: 50%; flex-shrink: 0; }
    .toast.ok .toast-dot  { background: var(--ok); box-shadow: 0 0 8px var(--ok); }
    .toast.err .toast-dot { background: var(--danger); box-shadow: 0 0 8px var(--danger); }
    .hidden { display: none; }
    .spacer { margin-top: 12px; }
  </style>
</head>
<body>
  <main class="wrap">
    <header class="topbar">
      <div class="brand">
        <h1>Smart Home IoT</h1>
        <span>Admin — Anti-Replay (Nonce)</span>
      </div>
      <div class="theme-seg" role="group" aria-label="Tema da interface">
        <button type="button" data-t="auto">Auto</button>
        <button type="button" data-t="light">Claro</button>
        <button type="button" data-t="dark">Escuro</button>
      </div>
    </header>

    <section class="card" id="statusCard">
      <h2>Estado do Controlador</h2>
      <div class="status-row">
        <div class="led-socket"><div class="led-core" id="led"></div></div>
        <div class="status-text">
          <strong id="estado">Carregando...</strong>
          <small id="conn">Sincronizando com o controlador</small>
        </div>
      </div>
      <div class="well hidden" id="eventoBox" style="margin-top:14px"><b>Último evento:</b> <span id="evento">-</span></div>
      <div class="well hidden" id="nonceBox" style="margin-top:10px"><b>Nonce atual:</b> <span class="mono" id="nonce">-</span></div>
    </section>

    <section class="card" id="loginCard">
      <h2>Autenticação</h2>
      <form id="loginForm">
        <label for="u">Usuário</label>
        <input id="u" name="usuario" type="text" autocomplete="username" placeholder="Digite o nome do usuário administrador" required>
        <label for="s">Senha</label>
        <input id="s" name="senha" type="password" autocomplete="current-password" placeholder="Digite a senha" required>
        <button type="submit" class="btn btn-primary" id="btnEntrar">Entrar</button>
      </form>
    </section>

    <section class="card hidden" id="painelCard">
      <h2>Controles do Administrador</h2>
      <div class="well">Token gerado para o Nonce <span class="mono" id="tokNonce">-</span> (uso único).</div>
      <div class="btn-grid">
        <button type="button" class="btn btn-primary" id="btnOn">Ligar Luz</button>
        <button type="button" class="btn btn-danger" id="btnOff">Apagar Luz</button>
      </div>
      <div class="spacer">
        <button type="button" class="btn btn-muted" id="btnSair">Reautenticar / Sair</button>
      </div>
    </section>

    <nav>
      <a class="btn btn-muted" href="/">Voltar para Convidado</a>
    </nav>
  </main>

  <div class="toast" id="toast" role="status" aria-live="polite">
    <span class="toast-dot"></span>
    <span id="toastTxt"></span>
  </div>

  <script>
    const led = document.getElementById('led');
    const estado = document.getElementById('estado');
    const conn = document.getElementById('conn');
    const eventoBox = document.getElementById('eventoBox');
    const nonceBox = document.getElementById('nonceBox');
    const evento = document.getElementById('evento');
    const nonceEl = document.getElementById('nonce');
    const tokNonce = document.getElementById('tokNonce');
    const loginCard = document.getElementById('loginCard');
    const painelCard = document.getElementById('painelCard');
    const loginForm = document.getElementById('loginForm');
    const toast = document.getElementById('toast');
    const toastTxt = document.getElementById('toastTxt');
    const btnEntrar = document.getElementById('btnEntrar');
    const btnOn = document.getElementById('btnOn');
    const btnOff = document.getElementById('btnOff');
    const btnSair = document.getElementById('btnSair');

    let adminToken = '';
    let ocupado = false;
    let luzAcesaAtual = null;
    let toastTimer = null;

    function sincronizarBotoes() {
      btnOn.disabled = ocupado || luzAcesaAtual === true;
      btnOff.disabled = ocupado || luzAcesaAtual === false;
    }

    function aplicarTema(mode) {
      localStorage.setItem('sh_theme', mode);
      if (mode === 'light' || mode === 'dark') {
        document.documentElement.setAttribute('data-theme', mode);
      } else {
        document.documentElement.removeAttribute('data-theme');
      }
      document.querySelectorAll('.theme-seg button').forEach(b => {
        b.classList.toggle('active', b.dataset.t === mode);
      });
    }
    document.querySelectorAll('.theme-seg button').forEach(b => {
      b.addEventListener('click', () => aplicarTema(b.dataset.t));
    });
    aplicarTema(localStorage.getItem('sh_theme') || 'auto');

    function mostrarToast(ok, texto) {
      toastTxt.textContent = texto;
      toast.className = 'toast show ' + (ok ? 'ok' : 'err');
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => toast.classList.remove('show'), 3200);
    }

    async function atualizarStatus() {
      if (ocupado) return;
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 1400);
      try {
        const r = await fetch('/status', { signal: c.signal, cache: 'no-store' });
        const d = await r.json();
        luzAcesaAtual = !!d.luz_acesa;
        led.classList.toggle('on', luzAcesaAtual);
        estado.textContent = luzAcesaAtual ? 'Luz acesa' : 'Luz apagada';
        conn.textContent = 'Controlador online (192.168.4.1)';
        if (adminToken) {
          evento.textContent = d.ultimo_evento || '-';
          nonceEl.textContent = d.nonce_atual || '-';
        }
        sincronizarBotoes();
      } catch (_) {
        conn.textContent = 'Sem resposta do controlador';
      } finally {
        clearTimeout(t);
      }
    }

    loginForm.addEventListener('submit', async e => {
      e.preventDefault();
      if (ocupado) return;
      ocupado = true;
      btnEntrar.disabled = true;
      const body = new URLSearchParams({
        usuario: document.getElementById('u').value.trim(),
        senha: document.getElementById('s').value
      });
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 2000);
      try {
        const r = await fetch('/admin', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString(),
          signal: c.signal
        });
        const j = await r.json();
        if (r.ok && j.ok) {
          adminToken = j.token || '';
          tokNonce.textContent = j.nonce || '-';
          loginCard.classList.add('hidden');
          eventoBox.classList.remove('hidden');
          nonceBox.classList.remove('hidden');
          painelCard.classList.remove('hidden');
          document.getElementById('s').value = '';
          mostrarToast(true, 'Sessão iniciada com sucesso');
        } else if (r.status === 403) {
          mostrarToast(false, 'Usuário ou senha inválidos');
        } else {
          mostrarToast(false, 'Erro inesperado ao autenticar');
        }
      } catch (_) {
        mostrarToast(false, 'Não foi possível conectar ao controlador');
      } finally {
        clearTimeout(t);
        ocupado = false;
        btnEntrar.disabled = false;
        atualizarStatus();
      }
    });

    async function enviarComando(acao) {
      if (ocupado) return;
      ocupado = true;
      btnOn.disabled = true;
      btnOff.disabled = true;
      const url = acao === 'OFF'
        ? '/luz?acao=OFF&user=admin&token=' + encodeURIComponent(adminToken)
        : '/luz?acao=ON&user=admin';
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 2000);
      try {
        const r = await fetch(url, { signal: c.signal, cache: 'no-store' });
        if (r.ok) {
          mostrarToast(true, acao === 'ON' ? 'Luz acesa com sucesso' : 'Luz apagada com sucesso');
        } else if (r.status === 403) {
          mostrarToast(false, acao === 'OFF' ? 'Token de uso único expirado. Reautentique.' : 'Sem permissão para esta ação');
        } else {
          mostrarToast(false, 'Erro inesperado ao comunicar com o controlador');
        }
      } catch (_) {
        mostrarToast(false, 'Não foi possível conectar ao controlador');
      } finally {
        clearTimeout(t);
        ocupado = false;
        sincronizarBotoes();
        atualizarStatus();
      }
    }

    btnOn.addEventListener('click', () => enviarComando('ON'));
    btnOff.addEventListener('click', () => enviarComando('OFF'));
    btnSair.addEventListener('click', () => {
      adminToken = '';
      eventoBox.classList.add('hidden');
      nonceBox.classList.add('hidden');
      evento.textContent = '-';
      painelCard.classList.add('hidden');
      loginCard.classList.remove('hidden');
      mostrarToast(true, 'Sessão encerrada');
    });

    atualizarStatus();
    setInterval(atualizarStatus, 2000);
  </script>
</body>
</html>
)rawliteral";
