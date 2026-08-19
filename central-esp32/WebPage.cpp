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
    <h1>Connexion</h1>
    <input id="username" type="text" value="admin" placeholder="Nom d'utilisateur" autocomplete="username">
    <input id="password" type="password" placeholder="Mot de passe" autocomplete="current-password">
    <button onclick="login()">Se connecter</button>
    <div class="error" id="error"></div>
  </div>

  <script>
    async function login() {
      const username = document.getElementById('username').value.trim();
      const password = document.getElementById('password').value;
      const res = await fetch('/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username, password })
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
    .posts-list-title{margin:24px 0 12px;color:#fff;font-size:22px}
    .posts-empty{margin:0;color:#cbd5e1;font-size:16px}
    .stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px;margin:18px 0}
    .stat-card,.panel{background:var(--surface);color:var(--text);border-radius:var(--radius);box-shadow:var(--shadow);padding:18px}
    .page-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(310px,1fr));gap:18px;align-items:start}
    .posts-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(230px,280px));gap:12px;align-items:start;justify-content:start}
    .post-card{background:#f1f5f9;color:var(--text);border-radius:12px;box-shadow:var(--shadow);padding:14px;border:1px solid #cbd5e1;transition:transform .15s ease,box-shadow .15s ease}
    .post-card:hover{transform:translateY(-2px);box-shadow:0 12px 26px rgba(15,23,42,.16)}
    .post-card.active{background:#dcfce7;border-color:#86efac}
    .post-card.idle{background:#e2e8f0;border-color:#94a3b8}
    .post-card.offline{background:#fee2e2;border-color:#fca5a5}
    .post-card.error{background:#ffedd5;border-color:#fdba74}
    .post-card.recovery_pending{background:#fef3c7;border-color:#fbbf24}
    .post-card.unknown{background:#f1f5f9;border-color:#cbd5e1}
    .post-header{display:flex;align-items:flex-start;justify-content:space-between;gap:8px;margin-bottom:7px}
    .post-title{font-size:16px;font-weight:700;line-height:1.25;margin:0}
    .meta{color:#475569;font-size:12px;margin:2px 0;word-break:break-word}
    .badge{padding:6px 10px;border-radius:999px;font-size:12px;font-weight:700;white-space:nowrap}
    .badge.active{background:var(--success-bg);color:var(--success-text)}
    .badge.idle{background:var(--idle-bg);color:var(--idle-text)}
    .badge.offline{background:var(--danger-bg);color:var(--danger-text)}
    .badge.error{background:var(--warning-bg);color:var(--warning-text)}
    .badge.recovery_pending{background:var(--warning-bg);color:var(--warning-text)}
    .badge.unknown{background:var(--idle-bg);color:var(--idle-text)}
    .post-card .badge{padding:4px 7px;background:rgba(255,255,255,.62);color:#334155;border:1px solid rgba(51,65,85,.12);font-size:10px}
    .timer{display:flex;align-items:flex-end;justify-content:space-between;gap:8px;margin:9px 0;padding:8px 10px;border-radius:9px;background:rgba(255,255,255,.55);border:1px solid rgba(51,65,85,.12)}
    .timer-label{font-size:11px;color:#475569}
    .timer-value{font-size:20px;font-weight:700;line-height:1}
    .relay{font-size:11px;font-weight:700;margin-bottom:8px}
    .relay.on{color:var(--success-text)}
    .relay.off{color:var(--danger-text)}
    .recovery-box{margin:8px 0 0;padding:8px;border-radius:9px;background:rgba(255,255,255,.48);color:var(--warning-text);border:1px solid rgba(146,64,14,.2)}
    .recovery-copy{font-size:12px;line-height:1.3;margin-bottom:7px}
    .post-card .actions{gap:6px}
    .post-card button.action{padding:8px 10px;border-radius:8px;font-size:12px}
    .post-card .coin-action{min-width:40px}
    .actions{display:flex;gap:8px;flex-wrap:wrap}
    button.action{border:none;border-radius:10px;padding:10px 14px;font-weight:700;cursor:pointer}
    button.action:disabled{opacity:.45;cursor:not-allowed}
    .primary{background:var(--accent);color:white}
    .secondary{background:#e2e8f0;color:#0f172a}
    .warn{background:#f59e0b;color:#fff}
    .danger{background:#dc2626;color:#fff}
    .form-group{display:flex;flex-direction:column;gap:6px;margin-bottom:12px}
    input,textarea,select{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--border);font-size:14px;background:#fff}
    textarea{min-height:220px;resize:vertical;font-family:monospace}
    .message{margin-top:10px;font-size:13px;color:var(--muted);min-height:18px}
    .pending-list{display:grid;gap:10px}
    .pending-item{border:1px solid var(--border);border-radius:12px;padding:12px;background:#f8fafc}
    .editor{margin-top:12px;display:none;border-top:1px solid #e5e7eb;padding-top:12px}
    .logs{height:62vh;min-height:360px;overflow:auto;background:#0f172a;color:#e2e8f0;border-radius:12px;padding:12px;font-family:monospace;font-size:12px}
    .log-item{padding:7px 0;border-bottom:1px solid rgba(255,255,255,.08)}
    .empty{background:rgba(255,255,255,.08);border:1px dashed rgba(255,255,255,.22);border-radius:14px;padding:20px;color:#cbd5e1}
    @media (max-width:720px){.container{padding:14px}.nav a{flex:1 1 auto;text-align:center}.topbar-actions{width:100%;display:flex;gap:8px}.topbar-actions button{flex:1}.posts-grid{grid-template-columns:repeat(auto-fit,minmax(220px,1fr))}}
  </style>
</head>
<body>
  <div class="container">
    <div class="hero">
      <div class="topbar">
        <div>
          <h1 style="margin:0 0 8px;">Central monnayeur</h1>
          <div id="currentUserLabel" style="color:#93c5fd;font-size:13px;margin-top:6px;"></div>
        </div>
        <div class="topbar-actions" style="display:flex;gap:8px;flex-wrap:wrap;">
          <button class="small-btn logout-btn" onclick="logout()">Déconnexion</button>
          <button class="small-btn danger-btn" data-admin-only hidden onclick="resetWifi()">Reset Wi-Fi</button>
        </div>
      </div>
      <nav class="nav">
        <a href="/" data-path="/">Home</a>
        <a href="/config" data-path="/config" data-admin-only hidden>Configuration</a>
        <a href="/logs" data-path="/logs" data-admin-only hidden>Logs</a>
        <a href="/postes" data-path="/postes" data-admin-only hidden>Gestion des postes</a>
        <a href="/users" data-path="/users" data-admin-only hidden>Utilisateurs</a>
        <a href="/security" data-path="/security" data-admin-only hidden>Mot de passe / Token</a>
      </nav>
    </div>

    <section class="view" id="view-home">
      <h2 class="page-title">Vue des postes</h2>
      <div class="stats" id="stats"></div>
      <h2 class="posts-list-title">Liste des postes</h2>
      <div id="posts" class="posts-grid"></div>
    </section>

    <section class="view" id="view-config">
      <h2 class="page-title">Configuration et import/export</h2>
      <div class="page-grid">
        <div class="panel">
          <h2>Configuration générale</h2>
          <div class="form-group">
            <label for="coinDurationMinutes">Durée par coin (minutes)</label>
            <input id="coinDurationMinutes" type="number" min="1" step="1" placeholder="Durée par coin (min)" />
          </div>
          <div class="form-group">
            <label for="pulsesPerCoin">Impulsions par coin</label>
            <input id="pulsesPerCoin" type="number" min="1" placeholder="Impulsions par coin" />
          </div>
          <div class="form-group">
            <label for="availableCoins">Coins disponibles</label>
            <input id="availableCoins" type="number" min="0" placeholder="Crédit disponible" />
          </div>
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

    <section class="view" id="view-postes">
      <h2 class="page-title">Gestion des postes</h2>
      <div class="page-grid">
        <div class="panel">
          <div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;">
            <h2 style="margin:0;">Postes configurés</h2>
            <button class="action primary" onclick="load()">Actualiser</button>
          </div>
          <div id="managedPosts" class="pending-list"></div>
          <div class="message" id="managedMessage"></div>
        </div>

        <div class="panel">
          <h2>Postes découverts</h2>
          <div id="pendingPosts" class="pending-list"></div>
          <div class="message" id="pendingMessage"></div>
        </div>
      </div>
    </section>

    <section class="view" id="view-users">
      <h2 class="page-title">Gestion des utilisateurs</h2>
      <div class="page-grid">
        <div class="panel">
          <h2>Ajouter un utilisateur</h2>
          <div class="form-group">
            <label for="createUsername">Nom d'utilisateur</label>
            <input id="createUsername" maxlength="32" placeholder="Nom d'utilisateur unique" />
          </div>
          <div class="form-group">
            <label for="createFirstName">Prénom</label>
            <input id="createFirstName" maxlength="64" placeholder="Prénom" />
          </div>
          <div class="form-group">
            <label for="createLastName">Nom</label>
            <input id="createLastName" maxlength="64" placeholder="Nom" />
          </div>
          <div class="form-group">
            <label for="createPassword">Mot de passe</label>
            <input id="createPassword" type="password" minlength="6" placeholder="6 caractères minimum" />
          </div>
          <div class="form-group">
            <label for="createRole">Rôle</label>
            <select id="createRole">
              <option value="user">Utilisateur simple</option>
              <option value="admin">Administrateur</option>
            </select>
          </div>
          <button class="action primary" onclick="createUser()">Ajouter</button>
          <div class="message" id="createUserMessage"></div>
        </div>

        <div class="panel">
          <div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;">
            <h2 style="margin:0;">Comptes existants</h2>
            <button class="action primary" onclick="loadUsers()">Actualiser</button>
          </div>
          <div id="usersList" class="pending-list"></div>
          <div class="message" id="usersMessage"></div>
        </div>
      </div>
    </section>

    <section class="view" id="view-security">
      <h2 class="page-title">Mot de passe et API token</h2>
      <div class="page-grid">
        <div class="panel">
          <h2>Votre mot de passe administrateur</h2>
          <div class="form-group">
            <input id="newPassword" type="password" minlength="6" placeholder="Nouveau mot de passe (6 caractères minimum)" />
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
      '/discover':'view-postes',
      '/postes':'view-postes',
      '/users':'view-users',
      '/security':'view-security'
    };
    let currentPath = viewsByPath[window.location.pathname] ? window.location.pathname : '/';
    let refreshTimer = null;
    let logsTimer = null;
    let currentRole = 'user';
    let currentUsername = '';
    let postNames = {};
    let testCoinDurationSeconds = 1800;
    const showTestPosts = true;
    const testPosts = [
      {chipId:'test-recovery-1',id:'__test_recovery_1',name:'Poste coupure 1',ip:'192.168.1.201',status:'recovery_pending',relay:false,remaining:0,recoveryPending:true,recoveryRemaining:15*60},
      {chipId:'test-recovery-2',id:'__test_recovery_2',name:'Poste coupure 2',ip:'192.168.1.202',status:'recovery_pending',relay:false,remaining:0,recoveryPending:true,recoveryRemaining:7*60+30},
      {chipId:'test-active-1',id:'__test_active_1',name:'Poste actif 1',ip:'192.168.1.203',status:'active',relay:true,remaining:24*60+20,recoveryPending:false,recoveryRemaining:0},
      {chipId:'test-active-2',id:'__test_active_2',name:'Poste actif 2',ip:'192.168.1.204',status:'active',relay:true,remaining:9*60+45,recoveryPending:false,recoveryRemaining:0},
      {chipId:'test-offline-1',id:'__test_offline_1',name:'Poste hors ligne 1',ip:'192.168.1.205',status:'offline',relay:false,remaining:0,recoveryPending:false,recoveryRemaining:0},
      {chipId:'test-offline-2',id:'__test_offline_2',name:'Poste hors ligne 2',ip:'192.168.1.206',status:'offline',relay:false,remaining:0,recoveryPending:false,recoveryRemaining:0},
      {chipId:'test-idle-1',id:'__test_idle_1',name:'Poste inactif 1',ip:'192.168.1.207',status:'idle',relay:false,remaining:0,recoveryPending:false,recoveryRemaining:0},
      {chipId:'test-idle-2',id:'__test_idle_2',name:'Poste inactif 2',ip:'192.168.1.208',status:'idle',relay:false,remaining:0,recoveryPending:false,recoveryRemaining:0}
    ];

    function findTestPost(postId) {
      return testPosts.find(post => post.id === postId);
    }

    function applyPermissions(data) {
      currentRole = data.accessRole || 'user';
      currentUsername = data.currentUser?.username || '';
      const isAdmin = currentRole === 'admin';
      document.querySelectorAll('[data-admin-only]').forEach(el => { el.hidden = !isAdmin; });

      const identity = data.currentUser;
      document.getElementById('currentUserLabel').textContent = identity
        ? `${identity.firstName} ${identity.lastName} — ${identity.role === 'admin' ? 'Administrateur' : 'Utilisateur simple'}`
        : 'Accès API';

      if (!isAdmin && currentPath !== '/') {
        startPage('/');
      }
    }

    function activateView() {
      const navPath = currentPath === '/discover' ? '/postes' : currentPath;
      document.querySelectorAll('.view').forEach(view => view.classList.remove('active'));
      document.getElementById(viewsByPath[currentPath]).classList.add('active');
      document.querySelectorAll('.nav a').forEach(link => link.classList.toggle('active', link.dataset.path === navPath));
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
      } else if (currentPath === '/postes' || currentPath === '/discover') {
        load();
      } else if (currentPath === '/users') {
        load();
        loadUsers();
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
      if (['active','idle','offline','error','recovery_pending'].includes(value)) return value;
      return 'unknown';
    }

    function statusLabel(status) {
      return ({
        active: 'Actif',
        idle: 'Inactif',
        offline: 'Hors ligne',
        error: 'Erreur',
        recovery_pending: 'Reprise en attente'
      })[(status || '').toLowerCase()] || status || 'Inconnu';
    }

    function setMessage(id, text, isError = false) {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = text;
      el.style.color = isError ? '#b91c1c' : '#64748b';
    }

    function friendlyError(message) {
      return ({
        'invalid username': "Nom d'utilisateur invalide (3 à 32 caractères : lettres, chiffres, point, tiret ou underscore).",
        'invalid name': 'Nom et prénom obligatoires.',
        'password too short': 'Le mot de passe doit contenir au moins 6 caractères.',
        'invalid role': 'Rôle invalide.',
        'username already exists': "Ce nom d'utilisateur existe déjà.",
        'user limit reached': "La limite d'utilisateurs est atteinte.",
        'user not found': 'Utilisateur introuvable.',
        'cannot delete own account': 'Vous ne pouvez pas supprimer votre propre compte.',
        'cannot change own role': 'Vous ne pouvez pas modifier votre propre rôle.',
        'last admin required': 'Au moins un administrateur doit être conservé.',
        'use password settings': 'Utilisez la page Sécurité pour modifier votre propre mot de passe.',
        'admin required': 'Cette action est réservée aux administrateurs.',
        'recovery pending': "Décidez d'abord de relancer ou d'annuler la session interrompue.",
        'no recovery pending': "Il n'y a plus de session interrompue à traiter."
      })[message] || message;
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

    function postLabel(postId) {
      return postNames[postId] || 'ce poste';
    }

    async function load() {
      const data = await api('/posts');
      applyPermissions(data);
      const realPosts = data.posts || [];
      const posts = showTestPosts ? [...realPosts, ...testPosts] : realPosts;
      testCoinDurationSeconds = Number(data.coinDurationSeconds || 1800);
      postNames = Object.fromEntries(posts.map(post => [post.id, post.name]));
      const activeCount = posts.filter(p => p.status === 'active').length;
      const offlineCount = posts.filter(p => p.status === 'offline').length;
      const recoveryCount = posts.filter(p => p.recoveryPending).length;

      document.getElementById('stats').innerHTML = `
        <div class="stat-card">
          <div>Coins disponibles</div>
          <div style="font-size:28px;font-weight:700;">${data.availableCoins}</div>
          ${currentRole === 'admin' ? '<button class="action primary" style="margin-top:10px;" onclick="simulateCoin()">+1 coin</button>' : ''}
        </div>
        <div class="stat-card"><div>Durée par coin</div><div style="font-size:28px;font-weight:700;">${formatTime(data.coinDurationSeconds)}</div></div>
        <div class="stat-card"><div>Postes actifs</div><div style="font-size:28px;font-weight:700;">${activeCount}</div></div>
        <div class="stat-card"><div>Postes hors ligne</div><div style="font-size:28px;font-weight:700;">${offlineCount}</div></div>
        <div class="stat-card"><div>Reprises en attente</div><div style="font-size:28px;font-weight:700;">${recoveryCount}</div></div>
      `;

      document.getElementById('coinDurationMinutes').value = Number(data.coinDurationSeconds || 1800) / 60;
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
          <div class="meta"><b>IP :</b> ${esc(p.ip)}</div>
          <button class="action primary" style="margin-top:8px;" onclick="configurePending('${esc(p.chipId)}')">Ajouter</button>
        </div>
      `).join('') : '<div class="empty">Aucun poste en attente.</div>';

      document.getElementById('posts').innerHTML = posts.length ? posts.map(p => `
        <div class="post-card ${badgeClass(p.status)}">
          <div class="post-header">
            <h3 class="post-title">${esc(p.name)}</h3>
            <span class="badge ${badgeClass(p.status)}">${esc(statusLabel(p.status))}</span>
          </div>
          <div class="meta"><b>IP :</b> ${esc(p.ip)}</div>
          <div class="timer">
            <div class="timer-label">${p.recoveryPending ? 'Temps sauvegardé' : 'Temps restant'}</div>
            <div class="timer-value">${formatTime(p.recoveryPending ? p.recoveryRemaining : p.remaining)}</div>
          </div>
          <div class="relay ${p.relay ? 'on' : 'off'}">Relais : ${p.relay ? 'ON' : 'OFF'}</div>
          ${p.recoveryPending ? `
            <div class="recovery-box">
              <div class="recovery-copy"><b>Coupure détectée.</b></div>
              <div class="actions">
                <button class="action primary" onclick="resumeRecovery('${esc(p.id)}')">Reprendre</button>
                <button class="action danger" onclick="cancelRecovery('${esc(p.id)}')">Annuler</button>
              </div>
            </div>
          ` : `
            <div class="actions">
              <button class="action primary coin-action" title="Affecter 1 coin de plus pour ce poste" aria-label="Affecter 1 coin de plus pour ce poste" onclick="assign('${esc(p.id)}',1)">+1</button>
              <button class="action primary coin-action" title="Affecter 2 coins de plus pour ce poste" aria-label="Affecter 2 coins de plus pour ce poste" onclick="assign('${esc(p.id)}',2)">+2</button>
              <button class="action secondary" title="Arrêter ce poste" onclick="stopPost('${esc(p.id)}')">Arrêter</button>
            </div>
          `}
        </div>
      `).join('') : '<p class="posts-empty">Aucun poste configuré.</p>';

      const managedPostsEl = document.getElementById('managedPosts');
      if (managedPostsEl) {
        managedPostsEl.innerHTML = realPosts.length ? realPosts.map(p => {
          const locked = p.status === 'active' || Number(p.remaining || 0) > 0 || p.recoveryPending;
          return `
            <div class="pending-item">
              <div style="display:flex;align-items:flex-start;justify-content:space-between;gap:10px;flex-wrap:wrap;">
                <div>
                  <div style="font-size:16px;font-weight:700;">${esc(p.name)}</div>
                  <div class="meta"><b>IP :</b> ${esc(p.ip)}</div>
                  <div class="meta"><b>Temps restant :</b> ${formatTime(p.remaining)}</div>
                  ${p.recoveryPending ? `<div class="meta"><b>Temps récupérable :</b> ${formatTime(p.recoveryRemaining)}</div>` : ''}
                </div>
                <span class="badge ${badgeClass(p.status)}">${esc(statusLabel(p.status))}</span>
              </div>
              <div class="actions" style="margin-top:10px;">
                <button class="action warn" onclick="pingPost('${esc(p.id)}')">Ping</button>
                <button class="action warn" ${locked ? 'disabled' : ''} onclick="toggleEdit('${esc(p.id)}')">Modifier</button>
                <button class="action danger" ${locked ? 'disabled' : ''} onclick="deletePost('${esc(p.id)}')">Supprimer</button>
              </div>
              ${locked ? '<div class="message">Modification et suppression disponibles uniquement si le poste est inactif et le timer est à 0.</div>' : ''}
              <div class="editor" id="editor-${esc(p.id)}">
                <div class="form-group"><input id="edit-name-${esc(p.id)}" value="${esc(p.name)}" /></div>
                <button class="action primary" onclick="updatePost('${esc(p.id)}')">Enregistrer modification</button>
              </div>
            </div>
          `;
        }).join('') : '<div class="empty">Aucun poste configuré.</div>';
      }
    }

    async function assign(postId, coins) {
      const testPost = findTestPost(postId);
      if (testPost) {
        if (testPost.status === 'offline') {
          alert('Ce poste de test est hors ligne.');
          return;
        }
        if (testPost.recoveryPending) {
          alert(friendlyError('recovery pending'));
          return;
        }
        testPost.status = 'active';
        testPost.relay = true;
        testPost.remaining += coins * testCoinDurationSeconds;
        load();
        return;
      }

      try {
        await api('/assign', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId, coins })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(friendlyError(e.message)); }
    }

    async function resumeRecovery(postId) {
      if (!confirm(`Reprendre le temps sauvegardé pour ${postLabel(postId)} ?`)) return;

      const testPost = findTestPost(postId);
      if (testPost) {
        testPost.status = 'active';
        testPost.relay = true;
        testPost.remaining = testPost.recoveryRemaining;
        testPost.recoveryPending = false;
        testPost.recoveryRemaining = 0;
        load();
        return;
      }

      try {
        await api('/recovery/resume', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(friendlyError(e.message)); }
    }

    async function cancelRecovery(postId) {
      if (!confirm(`Annuler définitivement le temps sauvegardé pour ${postLabel(postId)} ?`)) return;

      const testPost = findTestPost(postId);
      if (testPost) {
        testPost.status = 'idle';
        testPost.relay = false;
        testPost.remaining = 0;
        testPost.recoveryPending = false;
        testPost.recoveryRemaining = 0;
        load();
        return;
      }

      try {
        await api('/recovery/cancel', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(friendlyError(e.message)); }
    }

    async function simulateCoin() {
      try {
        await api('/coins/simulate', { method:'POST' });
        load();
        loadLogs();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function configurePending(chipId) {
      const name = prompt('Nom affiché du poste, ex: Poste 1');
      if (!name) return;

      try {
        await api('/poste/configure', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ chipId, name: name.trim() })
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
        setMessage('managedMessage', 'Poste modifié avec succès.');
        load();
      } catch(e) {
        if (e.message !== 'unauthorized') {
          setMessage('managedMessage', e.message, true);
          alert(e.message);
        }
      }
    }

    async function deletePost(id) {
      const yes = confirm(`Supprimer ${postLabel(id)} ?`);
      if (!yes) return;

      try {
        await api('/post/delete', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ id })
        });
        setMessage('managedMessage', 'Poste supprimé. Il réapparaîtra dans les postes découverts à sa prochaine annonce.');
        load();
      } catch(e) {
        if (e.message !== 'unauthorized') {
          setMessage('managedMessage', e.message, true);
          alert(e.message);
        }
      }
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
      const coinDurationMinutes = Number(document.getElementById('coinDurationMinutes').value || 0);
      const coinDurationSeconds = Math.round(coinDurationMinutes * 60);
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
      const testPost = findTestPost(postId);
      if (testPost) {
        if (testPost.status === 'offline') {
          alert('Ce poste de test est hors ligne.');
          return;
        }
        testPost.status = 'idle';
        testPost.relay = false;
        testPost.remaining = 0;
        load();
        return;
      }

      try {
        await api('/stop', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ post_id: postId })
        });
        load();
      } catch(e) { if (e.message !== 'unauthorized') alert(e.message); }
    }

    async function loadUsers() {
      try {
        const data = await api('/users/data');
        currentUsername = data.currentUsername || currentUsername;
        const users = data.users || [];
        document.getElementById('usersList').innerHTML = users.length ? users.map(user => {
          const isCurrent = user.username === currentUsername;
          return `
            <div class="pending-item">
              <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px;">
                <div><b>${esc(user.username)}</b>${isCurrent ? ' (vous)' : ''}</div>
                <span class="badge ${user.role === 'admin' ? 'active' : 'idle'}">${user.role === 'admin' ? 'Administrateur' : 'Utilisateur simple'}</span>
              </div>
              <div class="form-group">
                <label for="user-first-${user.username}">Prénom</label>
                <input id="user-first-${user.username}" maxlength="64" value="${esc(user.firstName)}" placeholder="Prénom" />
              </div>
              <div class="form-group">
                <label for="user-last-${user.username}">Nom</label>
                <input id="user-last-${user.username}" maxlength="64" value="${esc(user.lastName)}" placeholder="Nom" />
              </div>
              <div class="form-group">
                <label for="user-role-${user.username}">Rôle</label>
                <select id="user-role-${user.username}" ${isCurrent ? 'disabled' : ''}>
                  <option value="user" ${user.role === 'user' ? 'selected' : ''}>Utilisateur simple</option>
                  <option value="admin" ${user.role === 'admin' ? 'selected' : ''}>Administrateur</option>
                </select>
              </div>
              ${isCurrent ? '' : `
                <div class="form-group">
                  <label for="user-password-${user.username}">Nouveau mot de passe (facultatif)</label>
                  <input id="user-password-${user.username}" type="password" minlength="6" placeholder="6 caractères minimum" />
                </div>
              `}
              <div class="actions">
                <button class="action primary" onclick="updateUser('${user.username}', ${isCurrent})">Enregistrer</button>
                <button class="action danger" ${isCurrent ? 'disabled' : ''} onclick="deleteUser('${user.username}')">Supprimer</button>
              </div>
            </div>
          `;
        }).join('') : '<div class="empty">Aucun utilisateur.</div>';
      } catch(e) {
        if (e.message !== 'unauthorized') setMessage('usersMessage', friendlyError(e.message), true);
      }
    }

    async function createUser() {
      const username = document.getElementById('createUsername').value.trim();
      const firstName = document.getElementById('createFirstName').value.trim();
      const lastName = document.getElementById('createLastName').value.trim();
      const password = document.getElementById('createPassword').value;
      const role = document.getElementById('createRole').value;

      try {
        await api('/users/create', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ username, firstName, lastName, password, role })
        });
        ['createUsername','createFirstName','createLastName','createPassword'].forEach(id => {
          document.getElementById(id).value = '';
        });
        document.getElementById('createRole').value = 'user';
        setMessage('createUserMessage', 'Utilisateur ajouté avec succès.');
        loadUsers();
      } catch(e) {
        if (e.message !== 'unauthorized') setMessage('createUserMessage', friendlyError(e.message), true);
      }
    }

    async function updateUser(username, isCurrent) {
      const firstName = document.getElementById(`user-first-${username}`).value.trim();
      const lastName = document.getElementById(`user-last-${username}`).value.trim();
      const role = document.getElementById(`user-role-${username}`).value;
      const passwordInput = isCurrent ? null : document.getElementById(`user-password-${username}`);
      const password = passwordInput ? passwordInput.value : '';

      try {
        await api('/users/update', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ username, firstName, lastName, password, role })
        });
        setMessage('usersMessage', 'Utilisateur mis à jour.');
        loadUsers();
        load();
      } catch(e) {
        if (e.message !== 'unauthorized') setMessage('usersMessage', friendlyError(e.message), true);
      }
    }

    async function deleteUser(username) {
      if (!confirm(`Supprimer l'utilisateur ${username} ?`)) return;

      try {
        await api('/users/delete', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ username })
        });
        setMessage('usersMessage', 'Utilisateur supprimé.');
        loadUsers();
      } catch(e) {
        if (e.message !== 'unauthorized') setMessage('usersMessage', friendlyError(e.message), true);
      }
    }

    async function changePassword() {
      const password = document.getElementById('newPassword').value.trim();
      try {
        const data = await api('/auth/password', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({ password })
        });
        if (data.reauthenticate) {
          alert('Mot de passe mis à jour. Veuillez vous reconnecter.');
          window.location.href = '/login';
          return;
        }
      } catch(e) { if (e.message !== 'unauthorized') setMessage('authMessage', friendlyError(e.message), true); }
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
