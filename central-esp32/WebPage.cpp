#include "WebPage.h"

namespace WebPage {

  const char* loginPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Connexion admin</title>
  <style>
    body{font-family:Arial,sans-serif;background:#0f172a;margin:0;padding:20px;color:#fff}
    .card{max-width:420px;margin:60px auto;background:#fff;color:#0f172a;border-radius:18px;padding:20px;box-shadow:0 10px 30px rgba(0,0,0,.2)}
    input{width:100%;padding:12px;border-radius:12px;border:1px solid #ddd;box-sizing:border-box;margin-top:10px}
    button{margin-top:16px;width:100%;padding:12px;border:none;border-radius:12px;background:#2563eb;color:#fff;font-weight:bold;cursor:pointer}
    .error{color:#991b1b;margin-top:10px;font-size:14px}
  </style>
</head>
<body>
  <div class="card">
    <h1>Connexion admin</h1>
    <input id="password" type="password" placeholder="Mot de passe admin">
    <button onclick="login()">Se connecter</button>
    <div class="error" id="error"></div>
  </div>

  <script>
    async function login() {
      const password = document.getElementById('password').value;
      const res = await fetch('/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password })
      });

      const data = await res.json().catch(() => ({}));
      if (!res.ok) {
        document.getElementById('error').textContent = data.error || 'Erreur de connexion';
        return;
      }

      window.location.href = '/';
    }
  </script>
</body>
</html>
)rawliteral";
  }

  const char* dashboardPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta charset="UTF-8" />
  <title>Système monnayeur</title>
  <style>
    :root {
      --bg:#0f172a; --surface:#ffffff; --text:#0f172a; --muted:#64748b; --border:#e5e7eb;
      --accent:#2563eb; --accent-soft:#dbeafe; --success-bg:#dcfce7; --success-text:#166534;
      --warning-bg:#fef3c7; --warning-text:#92400e; --danger-bg:#fee2e2; --danger-text:#991b1b;
      --idle-bg:#e2e8f0; --idle-text:#334155; --shadow:0 10px 30px rgba(15,23,42,.12); --radius:14px;
    }
    *{box-sizing:border-box}
    body{margin:0;font-family:Arial,sans-serif;background:linear-gradient(180deg,var(--bg),#1e293b);color:white}
    .container{max-width:1280px;margin:0 auto;padding:20px}
    .hero{background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.08);border-radius:22px;padding:20px;box-shadow:var(--shadow);margin-bottom:18px}
    .topbar{display:flex;justify-content:space-between;align-items:flex-start;gap:14px;flex-wrap:wrap}
    .nav{display:flex;gap:8px;flex-wrap:wrap;margin-top:16px}
    .nav a{color:#e2e8f0;text-decoration:none;border:1px solid rgba(255,255,255,.14);border-radius:10px;padding:10px 12px;font-weight:700;font-size:14px}
    .nav a.active{background:#fff;color:#0f172a}
    .small-btn{border:none;border-radius:10px;padding:10px 14px;cursor:pointer;font-weight:700}
    .logout-btn{background:#e2e8f0;color:#0f172a}
    .danger-btn{background:#dc2626;color:#fff}
    .view{display:none}
    .view.active{display:block}
    .page-title{margin:0 0 14px;color:#fff}
    .stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px;margin:18px 0}
    .stat-card,.panel{background:var(--surface);color:var(--text);border-radius:var(--radius);box-shadow:var(--shadow);padding:18px}
    .page-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(310px,1fr));gap:18px;align-items:start}
    .posts-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:14px}
    .post-card{background:var(--surface);color:var(--text);border-radius:var(--radius);box-shadow:var(--shadow);padding:18px;border:1px solid var(--border)}
    .post-header{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:12px}
    .post-title{font-size:18px;font-weight:700;margin:0}
    .meta{color:var(--muted);font-size:13px;margin:4px 0;word-break:break-word}
    .badge{padding:6px 10px;border-radius:999px;font-size:12px;font-weight:700;white-space:nowrap}
    .badge.active{background:var(--success-bg);color:var(--success-text)}
    .badge.idle{background:var(--idle-bg);color:var(--idle-text)}
    .badge.offline{background:var(--danger-bg);color:var(--danger-text)}
    .badge.error{background:var(--warning-bg);color:var(--warning-text)}
    .badge.unknown{background:var(--idle-bg);color:var(--idle-text)}
    .timer{margin:14px 0;padding:12px;border-radius:12px;background:#f8fafc;border:1px solid var(--border)}
    .timer-value{font-size:24px;font-weight:700}
    .relay{font-size:13px;font-weight:700;margin-bottom:12px}
    .relay.on{color:var(--success-text)}
    .relay.off{color:var(--danger-text)}
    .actions{display:flex;gap:8px;flex-wrap:wrap}
    button.action{border:none;border-radius:10px;padding:10px 14px;font-weight:700;cursor:pointer}
    .primary{background:var(--accent);color:white}
    .secondary{background:#e2e8f0;color:#0f172a}
    .warn{background:#f59e0b;color:#fff}
    .danger{background:#dc2626;color:#fff}
    .form-group{display:flex;flex-direction:column;gap:6px;margin-bottom:12px}
    input,textarea{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--border);font-size:14px}
    textarea{min-height:220px;resize:vertical;font-family:monospace}
    .message{margin-top:10px;font-size:13px;color:var(--muted);min-height:18px}
    .pending-list{display:grid;gap:10px}
    .pending-item{border:1px solid var(--border);border-radius:12px;padding:12px;background:#f8fafc}
    .editor{margin-top:12px;display:none;border-top:1px solid #e5e7eb;padding-top:12px}
    .logs{height:62vh;min-height:360px;overflow:auto;background:#0f172a;color:#e2e8f0;border-radius:12px;padding:12px;font-family:monospace;font-size:12px}
    .log-item{padding:7px 0;border-bottom:1px solid rgba(255,255,255,.08)}
    .empty{background:rgba(255,255,255,.08);border:1px dashed rgba(255,255,255,.22);border-radius:14px;padding:20px;color:#cbd5e1}
    @media (max-width:720px){.container{padding:14px}.nav a{flex:1 1 auto;text-align:center}.topbar-actions{width:100%;display:flex;gap:8px}.topbar-actions button{flex:1}}
  </style>
</head>
<body>
  <div class="container">
    <div class="hero">
      <div class="topbar">
        <div>
          <h1 style="margin:0 0 8px;">Central monnayeur</h1>
          <div style="color:#cbd5e1;">Gestion des postes, crédits, configuration, logs et sécurité.</div>
        </div>
        <div class="topbar-actions" style="display:flex;gap:8px;flex-wrap:wrap;">
          <button class="small-btn logout-btn" onclick="logout()">Déconnexion</button>
          <button class="small-btn danger-btn" onclick="resetWifi()">Reset Wi-Fi</button>
        </div>
      </div>
      <nav class="nav">
        <a href="/" data-path="/">Home</a>
        <a href="/config" data-path="/config">Configuration</a>
        <a href="/logs" data-path="/logs">Logs</a>
        <a href="/discover" data-path="/discover">Découverte postes</a>
        <a href="/security" data-path="/security">Mot de passe / Token</a>
      </nav>
    </div>

    <section class="view" id="view-home">
      <h2 class="page-title">Vue des postes</h2>
      <div class="stats" id="stats"></div>
      <div id="posts" class="posts-grid"></div>
    </section>

    <section class="view" id="view-config">
      <h2 class="page-title">Configuration et import/export</h2>
      <div class="page-grid">
        <div class="panel">
          <h2>Configuration générale</h2>
          <div class="form-group"><input id="coinDurationSeconds" type="number" min="1" placeholder="Durée par coin (s)" /></div>
          <div class="form-group"><input id="pulsesPerCoin" type="number" min="1" placeholder="Impulsions par coin" /></div>
          <div class="form-group"><input id="availableCoins" type="number" min="0" placeholder="Crédit disponible" /></div>
          <button class="action primary" onclick="saveConfig()">Enregistrer</button>
          <div class="message" id="configMessage"></div>
        </div>

        <div class="panel">
          <h2>Export / Import</h2>
          <div class="actions">
            <button class="action primary" onclick="exportConfig()">Exporter JSON</button>
            <button class="action secondary" onclick="importConfig()">Importer JSON</button>
          </div>
          <div class="form-group" style="margin-top:12px;">
            <textarea id="configJson" placeholder="Colle ici le JSON d'export/import"></textarea>
          </div>
          <div class="message" id="importExportMessage"></div>
        </div>
      </div>
    </section>

    <section class="view" id="view-logs">
      <h2 class="page-title">Logs</h2>
      <div class="panel">
        <div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;">
          <h2 style="margin:0;">Journal d'événements</h2>
          <div class="actions">
            <button class="action primary" onclick="loadLogs()">Actualiser</button>
            <button class="action danger" onclick="clearLogs()">Vider</button>
          </div>
        </div>
        <div id="logs" class="logs"></div>
      </div>
    </section>

    <section class="view" id="view-discover">
      <h2 class="page-title">Découvrir et ajouter des postes</h2>
      <div class="panel">
        <h2>Postes non configurés</h2>
        <div id="pendingPosts" class="pending-list"></div>
        <div class="message" id="pendingMessage"></div>
      </div>
    </section>

    <section class="view" id="view-security">
      <h2 class="page-title">Mot de passe et API token</h2>
      <div class="page-grid">
        <div class="panel">
          <h2>Mot de passe admin</h2>
          <div class="form-group">
            <input id="newPassword" type="password" placeholder="Nouveau mot de passe admin" />
          </div>
          <button class="action primary" onclick="changePassword()">Changer mot de passe</button>
          <div class="message" id="authMessage"></div>
        </div>

        <div class="panel">
          <h2>Token API</h2>
          <div id="authInfo" style="font-size:14px;color:#475569;"></div>
          <button class="action warn" style="margin-top:14px;" onclick="regenerateToken()">Régénérer token API</button>
          <div class="message" id="tokenMessage"></div>
        </div>
      </div>
    </section>
  </div>

  <script>
    const viewsByPath = {
      '/':'view-home',
      '/config':'view-config',
      '/logs':'view-logs',
      '/discover':'view-discover',
      '/security':'view-security'
    };
    let currentPath = viewsByPath[window.location.pathname] ? window.location.pathname : '/';
    let refreshTimer = null;
    let logsTimer = null;

    function activateView() {
      document.querySelectorAll('.view').forEach(view => view.classList.remove('active'));
      document.getElementById(viewsByPath[currentPath]).classList.add('active');
      document.querySelectorAll('.nav a').forEach(link => link.classList.toggle('active', link.dataset.path === currentPath));
    }

    function stopRefreshTimers() {
      if (refreshTimer) {
        clearInterval(refreshTimer);
        refreshTimer = null;
      }
      if (logsTimer) {
        clearInterval(logsTimer);
        logsTimer = null;
      }
    }

    function startPage(path, pushState = true) {
      currentPath = viewsByPath[path] ? path : '/';
      if (pushState && window.location.pathname !== currentPath) {
        history.pushState({ path: currentPath }, '', currentPath);
      }

      activateView();
      stopRefreshTimers();

      if (currentPath === '/') {
        load();
        refreshTimer = setInterval(load, 3000);
      } else if (currentPath === '/logs') {
        loadLogs();
        logsTimer = setInterval(loadLogs, 5000);
      } else if (currentPath === '/discover') {
        load();
        refreshTimer = setInterval(load, 3000);
      } else {
        load();
      }
    }

    function setupNavigation() {
      document.querySelectorAll('.nav a').forEach(link => {
        link.addEventListener('click', event => {
          event.preventDefault();
          startPage(link.dataset.path || '/');
        });
      });

      window.addEventListener('popstate', () => {
        startPage(window.location.pathname, false);
      });
    }

    function formatTime(totalSeconds) {
      totalSeconds = Math.max(0, Number(totalSeconds || 0));
      const h = Math.floor(totalSeconds / 3600);
      const m = Math.floor((totalSeconds % 3600) / 60);
      const s = totalSeconds % 60;
      if (h > 0) return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(s).padStart(2,'0')}`;
      return `${String(m).padStart(2,'0')}:${String(s).padStart(2,'0')}`;
    }

    function badgeClass(status) {
      const value = (status || 'unknown').toLowerCase();
      if (['active','idle','offline','error'].includes(value)) return value;
      return 'unknown';
    }

    function setMessage(id, text, isError = false) {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = text;
      el.style.color = isError ? '#b91c1c' : '#64748b';
    }

    function esc(value) {
      return String(value ?? '').replace(/[&<>"']/g, ch => ({
        '&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;'
      })[ch]);
    }

    async function api(url, options = {}) {
      const res = await fetch(url, options);
      const data = await res.json().catch(() => ({}));
      if (res.status === 401) {
        window.location.href = '/login';
        throw new Error('unauthorized');
      }
      if (!res.ok) throw new Error(data.error || 'Erreur réseau');
      return data;
    }

    function toggleEdit(id) {
      const el = document.getElementById(`editor-${id}`);
      el.style.display = el.style.display === 'block' ? 'none' : 'block';
    }

    async function load() {
      const data = await api('/posts');
      const posts = data.posts || [];
      const activeCount = posts.filter(p => p.status === 'active').length;
      const offlineCount = posts.filter(p => p.status === 'offline').length;

      document.getElementById('stats').innerHTML = `
        <div class="stat-card">
          <div>Coins disponibles</div>
          <div style="font-size:28px;font-weight:700;">${data.availableCoins}</div>
          <button class="action primary" style="margin-top:10px;" onclick="simulateCoin()">+1 coin</button>
        </div>
        <div class="stat-card"><div>Durée par coin</div><div style="font-size:28px;font-weight:700;">${formatTime(data.coinDurationSeconds)}</div></div>
        <div class="stat-card"><div>Postes actifs</div><div style="font-size:28px;font-weight:700;">${activeCount}</div></div>
        <div class="stat-card"><div>Postes hors ligne</div><div style="font-size:28px;font-weight:700;">${offlineCount}</div></div>
      `;

      document.getElementById('coinDurationSeconds').value = data.coinDurationSeconds || 1800;
      document.getElementById('pulsesPerCoin').value = data.pulsesPerCoin || 1;
      document.getElementById('availableCoins').value = data.availableCoins || 0;
      document.getElementById('authInfo').innerHTML = `
        <b>Token API :</b> <code>${data.apiTokenMasked || ''}</code><br>
        <span style="font-size:12px;color:#64748b;">Header: Authorization: Bearer TON_TOKEN</span>
      `;

      const pendingPosts = data.pendingPosts || [];
      document.getElementById('pendingMessage').textContent = pendingPosts.length
        ? `${pendingPosts.length} poste(s) en attente de configuration.`
        : 'Aucun nouveau poste détecté.';
      document.getElementById('pendingPosts').innerHTML = pendingPosts.length ? pendingPosts.map(p => `
        <div class="pending-item">
          <div class="meta"><b>Chip :</b> ${esc(p.chipId)}</div>
          <div class="meta"><b>IP :</b> ${esc(p.ip)}</div>
          <button class="action primary" style="margin-top:8px;" onclick="configurePending('${esc(p.chipId)}')">Ajouter</button>
        </div>
      `).join('') : '<div class="empty">Aucun poste en attente.</div>';

      document.getElementById('posts').innerHTML = posts.length ? posts.map(p => `
        <div class="post-card">
          <div class="post-header">
            <h3 class="post-title">${esc(p.name)}</h3>
            <span class="badge ${badgeClass(p.status)}">${esc(p.status)}</span>
          </div>
          <div class="meta"><b>ID :</b> ${esc(p.id)}</div>
          <div class="meta"><b>IP :</b> ${esc(p.ip)}</div>
          <div class="timer">
            <div>Temps restant</div>
            <div class="timer-value">${formatTime(p.remaining)}</div>
          </div>
          <div class="relay ${p.relay ? 'on' : 'off'}">Relais : ${p.relay ? 'ON' : 'OFF'}</div>
          <div class="actions">
            <button class="action primary" onclick="assign('${esc(p.id)}',1)">+1 coin</button>
            <button class="action primary" onclick="assign('${esc(p.id)}',2)">+2 coins</button>
            <button class="action secondary" onclick="stopPost('${esc(p.id)}')">Arrêter</button>
            <button class="action warn" onclick="pingPost('${esc(p.id)}')">Ping</button>
            <button class="action warn" onclick="toggleEdit('${esc(p.id)}')">Modifier</button>
            <button class="action danger" onclick="deletePost('${esc(p.id)}')">Supprimer</button>
          </div>

          <div class="editor" id="editor-${esc(p.id)}">
            <div class="form-group"><input id="edit-name-${esc(p.id)}" value="${esc(p.name)}" /></div>
            <button class="action primary" onclick="updatePost('${esc(p.id)}')">Enregistrer modification</button>
          </div>
        </div>
      `).join('') : '<div class="empty">Aucun poste configuré.</div>';
    }

    async function assign(postId, coins) {
      try {
        await api('/assign', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId, coins })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function simulateCoin() {
      try {
        await api('/coins/simulate', { method:'POST' });
        load();
        loadLogs();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function configurePending(chipId) {
      const id = prompt('Identifiant du poste, ex: post1');
      if (!id) return;

      const name = prompt('Nom affiché du poste, ex: Poste 1');
      if (!name) return;

      try {
        await api('/poste/configure', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ chipId, id: id.trim(), name: name.trim() })
        });
        setMessage('pendingMessage', 'Poste configuré avec succès.');
        load();
      } catch(e) { if (e.message !== 'unauthorized') setMessage('pendingMessage', e.message, true); }
    }

    async function updatePost(id) {
      const name = document.getElementById(`edit-name-${id}`).value.trim();

      try {
        await api('/post/update', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ id, name })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function deletePost(id) {
      const yes = confirm(`Supprimer le poste ${id} ?`);
      if (!yes) return;

      try {
        await api('/post/delete', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ id })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function pingPost(id) {
      try {
        const data = await api('/post/ping', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ id })
        });
        alert(data.ok ? 'Ping réussi' : 'Ping échoué');
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function saveConfig() {
      const coinDurationSeconds = Number(document.getElementById('coinDurationSeconds').value || 0);
      const pulsesPerCoin = Number(document.getElementById('pulsesPerCoin').value || 0);
      const availableCoins = Number(document.getElementById('availableCoins').value || 0);

      try {
        await api('/config', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ coinDurationSeconds, pulsesPerCoin, availableCoins })
        });
        setMessage('configMessage', 'Configuration enregistrée.');
        load();
      } catch(e) { if (e.message !== 'unauthorized') setMessage('configMessage', e.message, true); }
    }

    async function stopPost(postId) {
      try {
        await api('/stop', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function changePassword() {
      const password = document.getElementById('newPassword').value.trim();
      try {
        await api('/auth/password', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ password })
        });
        document.getElementById('newPassword').value = '';
        setMessage('authMessage', 'Mot de passe mis à jour.');
      } catch(e) { if (e.message !== 'unauthorized') setMessage('authMessage', e.message, true); }
    }

    async function regenerateToken() {
      const yes = confirm("Régénérer le token API ? Les anciens clients devront être mis à jour.");
      if (!yes) return;

      try {
        const data = await api('/auth/token/regenerate', { method:'POST' });
        setMessage('tokenMessage', `Nouveau token: ${data.apiToken}`);
        load();
      } catch(e) { if (e.message !== 'unauthorized') setMessage('tokenMessage', e.message, true); }
    }

    async function exportConfig() {
      try {
        const data = await api('/config/export');
        document.getElementById('configJson').value = data.configJson || '';
        setMessage('importExportMessage', 'Configuration exportée.');
      } catch(e) { if (e.message !== 'unauthorized') setMessage('importExportMessage', e.message, true); }
    }

    async function importConfig() {
      const configJson = document.getElementById('configJson').value.trim();
      if (!configJson) {
        setMessage('importExportMessage', 'Le JSON est vide.', true);
        return;
      }

      const yes = confirm("Importer cette configuration et écraser la configuration actuelle ?");
      if (!yes) return;

      try {
        await api('/config/import', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ configJson })
        });
        setMessage('importExportMessage', 'Configuration importée avec succès.');
        load();
      } catch(e) { if (e.message !== 'unauthorized') setMessage('importExportMessage', e.message, true); }
    }

    async function loadLogs() {
      try {
        const data = await api('/logs/data');
        const html = (data.logs || []).map(log =>
          `<div class="log-item">[${esc(log.level)}] t=${esc(log.ts)} - ${esc(log.message)}</div>`
        ).join('');
        document.getElementById('logs').innerHTML = html || 'Aucun log.';
      } catch(e) {
        if (e.message !== 'unauthorized') document.getElementById('logs').textContent = e.message;
      }
    }

    async function clearLogs() {
      await api('/logs/clear', { method:'POST' });
      loadLogs();
    }

    async function logout() {
      await fetch('/logout', { method:'POST' });
      window.location.href = '/login';
    }

    async function resetWifi() {
      const yes = confirm("Supprimer la configuration Wi-Fi et redémarrer ?");
      if (!yes) return;
      await fetch('/wifi/reset', { method:'POST' });
      alert("Redémarrage en mode configuration Wi-Fi...");
    }

    setupNavigation();
    startPage(currentPath, false);
  </script>
</body>
</html>
)rawliteral";
  }
}
