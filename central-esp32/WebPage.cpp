#include "WebPage.h"

namespace WebPage {

  const char* loginPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="theme-color" content="#020817">
  <title>PS TIME MANAGER — Connexion</title>
  <style>
    :root {
      color-scheme:dark;
      --bg:#020817;
      --panel:#061021;
      --card:#081426;
      --border:#2a3950;
      --field:#091422;
      --text:#f7f9ff;
      --muted:#a9b1d3;
      --blue:#0877ff;
      --purple:#9138f4;
      --danger:#ff6b7a;
    }
    *{box-sizing:border-box}
    html,body{min-height:100%;margin:0}
    body{
      min-height:100vh;
      overflow-x:hidden;
      background:var(--bg);
      color:var(--text);
      font-family:Inter,-apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif;
      -webkit-font-smoothing:antialiased;
    }
    button,input{font:inherit}
    button,a,label{touch-action:manipulation}
    .login-shell{
      display:grid;
      grid-template-columns:50.4% 49.6%;
      min-height:100vh;
      background:#020817;
    }
    .visual-panel{
      position:relative;
      min-height:100vh;
      overflow:hidden;
      background-color:#020614;
      background-image:
        linear-gradient(180deg,rgba(1,5,18,.22) 0%,rgba(1,5,18,.02) 48%,rgba(1,5,18,.88) 100%),
        url('/assets/login-station-v2.jpg');
      background-position:center top;
      background-size:cover;
      border-right:1px solid rgba(104,128,180,.23);
    }
    .visual-panel::after{
      content:"";
      position:absolute;
      inset:0;
      pointer-events:none;
      background:
        linear-gradient(90deg,rgba(0,4,17,.04),rgba(0,4,17,0) 70%),
        radial-gradient(circle at 35% 30%,rgba(16,62,194,.08),transparent 43%);
    }
    .brand{
      position:absolute;
      z-index:1;
      top:36px;
      left:46px;
      width:min(535px,68%);
      height:auto;
      display:block;
      filter:drop-shadow(0 8px 18px rgba(0,0,0,.48));
      user-select:none;
      -webkit-user-drag:none;
    }
    .visual-copy{
      position:absolute;
      z-index:1;
      left:45px;
      right:38px;
      bottom:88px;
      max-width:600px;
      text-shadow:0 3px 18px rgba(0,0,0,.72);
    }
    .visual-copy h2{
      margin:0 0 12px;
      font-size:clamp(28px,2.1vw,36px);
      line-height:1.18;
      letter-spacing:-.02em;
      font-weight:650;
    }
    .visual-copy p{
      max-width:560px;
      margin:0;
      color:#cbd2ef;
      font-size:clamp(17px,1.35vw,22px);
      line-height:1.55;
    }
    .form-panel{
      position:relative;
      display:grid;
      place-items:center;
      min-width:0;
      min-height:100vh;
      padding:60px 64px;
      overflow:hidden;
      background:
        radial-gradient(circle at 50% 43%,rgba(20,48,100,.17),transparent 42%),
        linear-gradient(135deg,#020817 0%,#030b1a 55%,#020817 100%);
    }
    .form-panel::before{
      content:"";
      position:absolute;
      inset:0;
      opacity:.18;
      pointer-events:none;
      background-image:linear-gradient(rgba(61,98,160,.05) 1px,transparent 1px),linear-gradient(90deg,rgba(61,98,160,.04) 1px,transparent 1px);
      background-size:56px 56px;
      mask-image:linear-gradient(to bottom,transparent,#000 38%,transparent 88%);
    }
    .login-card{
      position:relative;
      z-index:1;
      width:min(536px,100%);
      min-height:630px;
      padding:31px 39px 25px;
      border:1px solid rgba(87,107,143,.52);
      border-radius:15px;
      background:linear-gradient(145deg,rgba(11,25,46,.96),rgba(5,15,31,.96));
      box-shadow:0 28px 70px rgba(0,0,0,.32),inset 0 1px 0 rgba(255,255,255,.025);
      transform:translateX(-15px);
    }
    .lock-badge{
      display:grid;
      place-items:center;
      width:77px;
      height:77px;
      margin:0 auto 17px;
      border:1px solid rgba(94,66,235,.36);
      border-radius:50%;
      color:#dfe7ff;
      background:
        radial-gradient(circle at 42% 30%,rgba(33,107,255,.48),transparent 50%),
        linear-gradient(145deg,rgba(17,70,180,.58),rgba(105,35,174,.6));
      box-shadow:0 0 35px rgba(88,52,231,.2),inset 0 0 20px rgba(128,72,255,.22);
    }
    .lock-badge svg{width:31px;height:31px}
    .login-card h1{
      margin:0;
      text-align:center;
      font-size:42px;
      line-height:1.15;
      letter-spacing:-.035em;
      font-weight:650;
    }
    .subtitle{
      margin:4px 0 26px;
      color:var(--muted);
      text-align:center;
      font-size:17px;
      line-height:1.45;
    }
    .field-group{margin-bottom:19px}
    .field-group label{
      display:block;
      margin:0 0 8px;
      color:#f1f4fb;
      font-size:16px;
      line-height:1.2;
    }
    .field-shell{
      position:relative;
      display:flex;
      align-items:center;
      height:57px;
      border:1px solid rgba(82,99,127,.43);
      border-radius:10px;
      background:linear-gradient(180deg,rgba(12,25,42,.95),rgba(8,19,33,.95));
      transition:border-color .18s ease,box-shadow .18s ease,background .18s ease;
    }
    .field-shell:focus-within{
      border-color:rgba(43,121,255,.78);
      background:#0a1728;
      box-shadow:0 0 0 3px rgba(28,101,255,.13),0 0 22px rgba(24,79,205,.08);
    }
    .field-icon{
      flex:0 0 auto;
      width:23px;
      height:23px;
      margin-left:19px;
      color:#7f8eac;
    }
    .field-shell input{
      width:100%;
      min-width:0;
      height:100%;
      padding:0 18px;
      border:0;
      outline:0;
      color:#f7f9ff;
      background:transparent;
      font-size:18px;
      caret-color:#5d8fff;
    }
    .field-shell input::placeholder{color:#71809c}
    .password-input{padding-right:54px!important}
    .password-toggle{
      position:absolute;
      right:6px;
      top:50%;
      display:grid;
      place-items:center;
      width:44px;
      height:44px;
      padding:0;
      border:0;
      border-radius:8px;
      color:#8ea0ca;
      background:transparent;
      cursor:pointer;
      transform:translateY(-50%);
      transition:color .15s ease,background .15s ease;
    }
    .password-toggle:hover,.password-toggle:focus-visible{color:#c7d4ff;background:rgba(62,92,157,.15);outline:0}
    .password-toggle svg{width:24px;height:24px}
    .password-toggle .eye-off{display:none}
    .password-toggle[aria-pressed="true"] .eye-on{display:none}
    .password-toggle[aria-pressed="true"] .eye-off{display:block}
    .form-options{
      display:flex;
      align-items:center;
      gap:18px;
      margin:-1px 0 20px;
      color:#c3cae2;
      font-size:15px;
    }
    .remember{
      display:inline-flex;
      align-items:center;
      gap:10px;
      min-width:0;
      cursor:pointer;
      user-select:none;
    }
    .remember input{
      position:absolute;
      width:1px;
      height:1px;
      opacity:0;
      pointer-events:none;
    }
    .checkmark{
      position:relative;
      flex:0 0 auto;
      width:22px;
      height:22px;
      border:1px solid #51627f;
      border-radius:5px;
      background:#0b192b;
      box-shadow:inset 0 0 0 1px rgba(255,255,255,.025);
      transition:.16s ease;
    }
    .remember input:checked + .checkmark{
      border-color:#247cff;
      background:linear-gradient(145deg,#2485ff,#075ae6);
      box-shadow:0 0 12px rgba(20,112,255,.25);
    }
    .remember input:checked + .checkmark::after{
      content:"";
      position:absolute;
      left:7px;
      top:3px;
      width:5px;
      height:10px;
      border:solid white;
      border-width:0 2px 2px 0;
      transform:rotate(45deg);
    }
    .remember input:focus-visible + .checkmark{outline:2px solid #7aa7ff;outline-offset:2px}
    .submit-btn{
      position:relative;
      display:flex;
      align-items:center;
      justify-content:center;
      gap:22px;
      width:100%;
      height:61px;
      padding:0 28px;
      border:0;
      border-radius:8px;
      color:white;
      background:linear-gradient(105deg,#0878ff 0%,#2057ff 45%,#9239f1 100%);
      box-shadow:0 12px 28px rgba(36,53,223,.22),inset 0 1px 0 rgba(255,255,255,.22);
      font-size:19px;
      font-weight:540;
      cursor:pointer;
      overflow:hidden;
      transition:transform .16s ease,filter .16s ease,box-shadow .16s ease;
    }
    .submit-btn::before{
      content:"";
      position:absolute;
      inset:0;
      background:linear-gradient(120deg,transparent 30%,rgba(255,255,255,.16),transparent 70%);
      transform:translateX(-120%);
      transition:transform .55s ease;
    }
    .submit-btn:hover{filter:brightness(1.08);box-shadow:0 15px 34px rgba(42,62,235,.3);transform:translateY(-1px)}
    .submit-btn:hover::before{transform:translateX(120%)}
    .submit-btn:active{transform:translateY(0)}
    .submit-btn:focus-visible{outline:3px solid rgba(111,155,255,.55);outline-offset:3px}
    .submit-btn:disabled{opacity:.72;cursor:wait;transform:none}
    .submit-btn svg{width:27px;height:27px;transition:transform .16s ease}
    .submit-btn:not(:disabled):hover svg{transform:translateX(4px)}
    .error{
      min-height:0;
      margin:0;
      color:var(--danger);
      text-align:center;
      font-size:13px;
      line-height:1.35;
    }
    .error:not(:empty){min-height:19px;margin:9px 0 -2px}
    .security-note{
      display:flex;
      align-items:center;
      justify-content:center;
      gap:11px;
      margin-top:16px;
      padding-top:18px;
      border-top:1px solid rgba(75,94,126,.36);
      color:#aeb8dc;
      font-size:15px;
    }
    .security-note svg{width:28px;height:28px;color:#aebcff}
    .product-footer{
      position:absolute;
      z-index:1;
      right:42px;
      bottom:40px;
      color:#8894bc;
      text-align:right;
      font-size:16px;
      letter-spacing:.01em;
    }
    .product-footer strong{color:#aeb9e5;font-weight:600;letter-spacing:.04em}
    @media (max-width:1100px){
      .login-shell{grid-template-columns:44% 56%}
      .form-panel{padding-left:40px;padding-right:40px}
      .brand{left:34px;width:72%}
      .visual-copy{left:34px}
      .login-card{transform:none}
    }
    @media (max-width:780px){
      .login-shell{display:block}
      .visual-panel{display:none}
      .form-panel{min-height:100vh;padding:30px 20px 86px}
      .login-card{min-height:0;padding:27px 25px 23px}
      .product-footer{left:20px;right:20px;bottom:24px;text-align:center;font-size:13px}
    }
    @media (max-width:480px){
      .login-card h1{font-size:34px}
      .subtitle{font-size:15px}
      .login-card{padding-left:20px;padding-right:20px}
    }
    @media (max-height:760px) and (min-width:781px){
      .form-panel{padding-top:24px;padding-bottom:72px}
      .login-card{min-height:0;padding-top:22px;padding-bottom:20px}
      .lock-badge{width:62px;height:62px;margin-bottom:10px}
      .login-card h1{font-size:34px}
      .subtitle{margin-bottom:16px}
      .field-group{margin-bottom:13px}
      .product-footer{bottom:20px}
      .visual-copy{bottom:52px}
    }
    @media (prefers-reduced-motion:reduce){*{scroll-behavior:auto!important;transition:none!important}}
  </style>
</head>
<body>
  <main class="login-shell">
    <section class="visual-panel" aria-label="Interface PS TIME MANAGER pour salle PlayStation">
      <img class="brand" src="/assets/game-room-logo-v4.webp" alt="PS TIME MANAGER">
      <div class="visual-copy">
        <h2>Pilotez votre salle de jeu</h2>
        <p>Gérez les postes, les sessions et les crédits<br>depuis une interface centrale sécurisée.</p>
      </div>
    </section>

    <section class="form-panel">
      <form class="login-card" id="loginForm" novalidate>
        <div class="lock-badge" aria-hidden="true">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
            <rect x="4.5" y="10" width="15" height="11" rx="2"></rect>
            <path d="M8 10V7.2a4 4 0 0 1 8 0V10"></path>
            <path d="M12 14.5v2"></path>
          </svg>
        </div>

        <h1>Connexion</h1>
        <p class="subtitle">Accédez à l’interface d’administration</p>

        <div class="field-group">
          <label for="username">Nom d’utilisateur</label>
          <div class="field-shell">
            <svg class="field-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
              <circle cx="12" cy="7.2" r="4"></circle>
              <path d="M4.8 21v-2.2a7.2 7.2 0 0 1 14.4 0V21z"></path>
            </svg>
            <input id="username" name="username" type="text" value="admin" placeholder="Nom d’utilisateur" autocomplete="username" spellcheck="false" required>
          </div>
        </div>

        <div class="field-group">
          <label for="password">Mot de passe</label>
          <div class="field-shell">
            <svg class="field-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
              <rect x="4.5" y="10" width="15" height="11" rx="2"></rect>
              <path d="M8 10V7.2a4 4 0 0 1 8 0V10"></path>
            </svg>
            <input class="password-input" id="password" name="password" type="password" placeholder="••••••••••••" autocomplete="current-password" required>
            <button class="password-toggle" id="passwordToggle" type="button" aria-label="Afficher le mot de passe" aria-pressed="false" title="Afficher le mot de passe">
              <svg class="eye-on" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                <path d="M2.5 12s3.5-6 9.5-6 9.5 6 9.5 6-3.5 6-9.5 6-9.5-6-9.5-6z"></path>
                <circle cx="12" cy="12" r="2.7"></circle>
              </svg>
              <svg class="eye-off" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                <path d="M3 3l18 18"></path>
                <path d="M10.6 6.2A10.5 10.5 0 0 1 12 6c6 0 9.5 6 9.5 6a16 16 0 0 1-3 3.7"></path>
                <path d="M6.1 6.2C3.8 8 2.5 12 2.5 12s3.5 6 9.5 6a9.4 9.4 0 0 0 3-.5"></path>
              </svg>
            </button>
          </div>
        </div>

        <div class="form-options">
          <label class="remember">
            <input id="remember" type="checkbox" checked>
            <span class="checkmark" aria-hidden="true"></span>
            <span>Se souvenir de moi</span>
          </label>
        </div>

        <button class="submit-btn" id="submitButton" type="submit">
          <span id="submitLabel">Se connecter</span>
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
            <path d="M5 12h14"></path>
            <path d="m14 7 5 5-5 5"></path>
          </svg>
        </button>
        <div class="error" id="error" role="alert" aria-live="polite"></div>

        <div class="security-note">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
            <path d="M12 3 4.5 6v5.3c0 4.8 3.2 8.2 7.5 9.7 4.3-1.5 7.5-4.9 7.5-9.7V6z"></path>
            <path d="m8.8 12 2.1 2.1 4.4-4.5"></path>
          </svg>
          <span>Connexion locale sécurisée</span>
        </div>
      </form>

      <footer class="product-footer"><strong>PS TIME MANAGER</strong> · Gestion centralisée du temps de jeu</footer>
    </section>
  </main>

  <script>
    const form = document.getElementById('loginForm');
    const usernameInput = document.getElementById('username');
    const passwordInput = document.getElementById('password');
    const rememberInput = document.getElementById('remember');
    const passwordToggle = document.getElementById('passwordToggle');
    const submitButton = document.getElementById('submitButton');
    const submitLabel = document.getElementById('submitLabel');
    const errorBox = document.getElementById('error');
    const rememberedUsername = localStorage.getItem('gameroom.rememberedUsername');

    if (rememberedUsername) {
      usernameInput.value = rememberedUsername;
      rememberInput.checked = true;
    }

    passwordToggle.addEventListener('click', () => {
      const showPassword = passwordInput.type === 'password';
      passwordInput.type = showPassword ? 'text' : 'password';
      passwordToggle.setAttribute('aria-pressed', String(showPassword));
      passwordToggle.setAttribute('aria-label', showPassword ? 'Masquer le mot de passe' : 'Afficher le mot de passe');
      passwordToggle.title = showPassword ? 'Masquer le mot de passe' : 'Afficher le mot de passe';
      passwordInput.focus({ preventScroll:true });
    });

    form.addEventListener('submit', async event => {
      event.preventDefault();
      errorBox.textContent = '';

      const username = usernameInput.value.trim();
      const password = passwordInput.value;
      const remember = rememberInput.checked;

      if (!username || !password) {
        errorBox.textContent = 'Saisissez votre nom d’utilisateur et votre mot de passe.';
        (!username ? usernameInput : passwordInput).focus();
        return;
      }

      submitButton.disabled = true;
      submitLabel.textContent = 'Connexion…';

      try {
        const response = await fetch('/login', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({username,password,remember})
        });
        const data = await response.json().catch(() => ({}));

        if (!response.ok) {
          throw new Error(data.error || 'Erreur de connexion');
        }

        if (remember) {
          localStorage.setItem('gameroom.rememberedUsername',username);
        } else {
          localStorage.removeItem('gameroom.rememberedUsername');
        }
        window.location.href = '/';
      } catch (error) {
        errorBox.textContent = error.message || 'Impossible de joindre le système central.';
        passwordInput.select();
      } finally {
        submitButton.disabled = false;
        submitLabel.textContent = 'Se connecter';
      }
    });

    window.addEventListener('load', () => {
      if (usernameInput.value) passwordInput.focus();
    });
  </script>
</body>
</html>
)rawliteral";
  }

  const char* dashboardPage() {
    return R"rawliteral(
<!DOCTYPE html><html lang="fr"><head><meta name="viewport" content="width=device-width, initial-scale=1" /><meta charset="UTF-8" /><meta name="theme-color" content="#020817" /><title>PS TIME MANAGER — Console</title><style>:root{color-scheme:dark;--bg:#020817;--sidebar:#020b1a;--surface:#071426;--surface-2:#0a192c;--surface-3:#0c1e33;--text:#f5f8ff;--muted:#8fa3bd;--border:#20364e;--border-bright:#34536f;--cyan:#00d9ff;--blue:#0877ff;--green:#20e3a2;--amber:#ffb31f;--red:#ff5563;--slate:#9ab2cc;--shadow:0 24px 60px rgba(0,0,0,.28);--radius:14px;}*{box-sizing:border-box}html{min-height:100%;background:var(--bg)}body{min-height:100vh;margin:0;overflow-x:hidden;color:var(--text);background:radial-gradient(circle at 75% 0%,rgba(8,119,255,.09),transparent 35%),linear-gradient(145deg,#020817 0%,#041022 58%,#020817 100%);font-family:Inter,-apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif;-webkit-font-smoothing:antialiased;}body::before{content:"";position:fixed;inset:0;z-index:-1;pointer-events:none;opacity:.14;background-image:linear-gradient(rgba(45,91,139,.09) 1px,transparent 1px),linear-gradient(90deg,rgba(45,91,139,.07) 1px,transparent 1px);background-size:64px 64px;mask-image:linear-gradient(90deg,transparent 15%,#000 62%,transparent 100%);}button,input,textarea,select{font:inherit}button,a{touch-action:manipulation}button:focus-visible,a:focus-visible,input:focus-visible,textarea:focus-visible,select:focus-visible{outline:2px solid var(--cyan);outline-offset:2px}[hidden]{display:none!important}.app-shell{display:grid;grid-template-columns:238px minmax(0,1fr);min-height:100vh}.sidebar{position:sticky;top:0;z-index:30;display:flex;flex-direction:column;width:238px;height:100vh;padding:18px 14px 14px;overflow-y:auto;border-right:1px solid rgba(53,83,113,.64);background:radial-gradient(circle at 40% 9%,rgba(0,217,255,.07),transparent 21%),linear-gradient(180deg,rgba(2,11,26,.98),rgba(3,16,34,.98));box-shadow:18px 0 50px rgba(0,0,0,.2);}.brand-wrap{padding:0 11px 14px;border-bottom:1px solid rgba(42,68,95,.55)}.brand-logo{display:block;width:100%;max-width:190px;height:auto;margin:0 auto;filter:drop-shadow(0 8px 18px rgba(0,0,0,.48));user-select:none;-webkit-user-drag:none}.brand-caption{margin:5px 0 0;text-align:center;color:#7189a4;font-size:10px;font-weight:700;letter-spacing:.19em;text-transform:uppercase}.nav{display:flex;flex:1;flex-direction:column;gap:4px;margin-top:14px}.nav-label{margin:0 12px 9px;color:#617994;font-size:10px;font-weight:800;letter-spacing:.16em;text-transform:uppercase}.nav a{position:relative;display:flex;align-items:center;gap:12px;min-height:46px;padding:0 13px;overflow:hidden;border:1px solid transparent;border-radius:9px;color:#aebdd0;text-decoration:none;font-size:14px;font-weight:650;transition:color .18s ease,border-color .18s ease,background .18s ease,transform .18s ease;}.nav a::before{content:"";position:absolute;left:-1px;top:8px;bottom:8px;width:2px;border-radius:2px;background:transparent;box-shadow:none}.nav a:hover{color:#eaf6ff;border-color:rgba(33,80,119,.55);background:rgba(5,34,60,.62);transform:translateX(2px)}.nav a.active{color:#fff;border-color:#16476b;background:linear-gradient(90deg,rgba(2,55,91,.94),rgba(4,31,55,.82));box-shadow:inset 0 0 20px rgba(0,181,255,.04)}.nav a.active::before{background:var(--cyan);box-shadow:0 0 13px rgba(0,217,255,.8)}.nav-icon{flex:0 0 auto;width:21px;height:21px;color:#8fa5bf}.nav a.active .nav-icon{color:var(--cyan);filter:drop-shadow(0 0 6px rgba(0,217,255,.48))}.sidebar-footer{display:grid;gap:8px;margin-top:14px;padding-top:12px;border-top:1px solid rgba(42,68,95,.55)}.user-panel{display:flex;align-items:center;gap:10px;min-width:0;padding:9px;border:1px solid rgba(41,69,98,.58);border-radius:10px;background:rgba(8,24,43,.68)}.user-avatar{display:grid;place-items:center;flex:0 0 auto;width:35px;height:35px;border:1px solid rgba(0,217,255,.34);border-radius:50%;color:#dffaff;background:linear-gradient(145deg,rgba(8,119,255,.36),rgba(145,56,244,.26));font-size:12px;font-weight:800;box-shadow:0 0 18px rgba(8,119,255,.12)}.user-copy{min-width:0;line-height:1.25}.user-name{overflow:hidden;color:#eef7ff;font-size:12px;font-weight:750;text-overflow:ellipsis;white-space:nowrap}.user-role{margin-top:3px;color:#7891ad;font-size:10px}.sidebar-action{display:flex;align-items:center;justify-content:center;gap:8px;width:100%;min-height:39px;padding:8px 10px;border:1px solid #284059;border-radius:8px;color:#b7c6d8;background:#09182a;font-size:12px;font-weight:700;cursor:pointer;transition:border-color .18s ease,color .18s ease,background .18s ease}.sidebar-action:hover{color:#fff;border-color:#3b6689;background:#0c2036}.sidebar-action.danger-btn{color:#ff9ba4;border-color:rgba(255,85,99,.25);background:rgba(87,16,28,.2)}.sidebar-action.danger-btn:hover{color:#ffd9dc;border-color:rgba(255,85,99,.5);background:rgba(109,18,32,.36)}.sidebar-action svg{width:16px;height:16px}.workspace{min-width:0}.topbar{position:sticky;top:0;z-index:20;display:flex;align-items:center;justify-content:space-between;gap:22px;min-height:82px;padding:14px clamp(22px,3vw,42px);border-bottom:1px solid rgba(42,67,92,.7);background:rgba(2,8,23,.84);box-shadow:0 14px 38px rgba(0,0,0,.12);backdrop-filter:blur(18px);-webkit-backdrop-filter:blur(18px);}.topbar-title{min-width:0}.topbar-title h1{margin:2px 0 0;color:#f7f9ff;font-size:clamp(22px,2vw,29px);line-height:1.15;letter-spacing:-.02em}.eyebrow{color:#6e87a2;font-size:10px;font-weight:800;letter-spacing:.18em;text-transform:uppercase}.topbar-meta{display:flex;align-items:center;gap:16px;color:#9eb0c4}.live-pill,.clock{display:flex;align-items:center;gap:8px;min-height:34px;padding:0 11px;border:1px solid rgba(43,71,99,.62);border-radius:8px;background:rgba(7,21,39,.72);font-size:12px;font-variant-numeric:tabular-nums}.live-dot{width:7px;height:7px;border-radius:50%;background:var(--green);box-shadow:0 0 10px rgba(32,227,162,.75)}.clock svg{width:16px;height:16px;color:#8da4be}.mobile-menu{display:none;width:42px;height:42px;padding:0;border:1px solid #29445f;border-radius:9px;color:#d7e8f8;background:#08182a;cursor:pointer}.mobile-menu svg{width:21px;height:21px}.sidebar-overlay{position:fixed;inset:0;z-index:25;display:none;background:rgba(0,4,13,.7);backdrop-filter:blur(3px)}.content{width:100%;max-width:1700px;margin:0 auto;padding:clamp(24px,3vw,42px)}.view{display:none}.view.active{display:block}.page-title{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0,0,0,0);white-space:nowrap;border:0}.section-heading{display:flex;align-items:end;justify-content:space-between;gap:16px;margin:38px 0 15px}.posts-list-title{margin:0;color:#f6f8fd;font-size:20px;line-height:1.2;letter-spacing:-.015em}.section-kicker{margin:5px 0 0;color:#7087a1;font-size:12px}.posts-empty{margin:0;color:#91a4ba;font-size:15px}.stats{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));gap:14px;margin:0}.stat-card{position:relative;min-height:127px;padding:17px;overflow:hidden;border:1px solid #20374f;border-radius:12px;color:var(--text);background:linear-gradient(145deg,rgba(8,24,43,.94),rgba(5,17,32,.94));box-shadow:0 14px 34px rgba(0,0,0,.2),inset 0 1px 0 rgba(255,255,255,.025);}.stat-card::after{content:"";position:absolute;right:-36px;bottom:-54px;width:120px;height:120px;border-radius:50%;background:var(--stat-color);opacity:.055;filter:blur(2px)}.stat-top{display:flex;align-items:center;gap:11px;color:#b7c5d6;font-size:12px;line-height:1.3}.stat-icon{display:grid;place-items:center;flex:0 0 auto;width:38px;height:38px;border:1px solid color-mix(in srgb,var(--stat-color) 66%,#1d354a);border-radius:10px;color:var(--stat-color);background:color-mix(in srgb,var(--stat-color) 7%,transparent);box-shadow:0 0 20px color-mix(in srgb,var(--stat-color) 9%,transparent)}.stat-icon svg{width:21px;height:21px}.stat-value{position:relative;z-index:1;margin:10px 0 0;color:var(--stat-color);font-family:"SFMono-Regular",Consolas,"Liberation Mono",monospace;font-size:31px;font-weight:800;line-height:1;letter-spacing:-.045em;text-shadow:0 0 18px color-mix(in srgb,var(--stat-color) 24%,transparent);font-variant-numeric:tabular-nums}.stat-card.coins{--stat-color:var(--amber)}.stat-card.duration{--stat-color:var(--cyan)}.stat-card.online{--stat-color:var(--green)}.stat-card.disconnected{--stat-color:var(--red)}.stat-card.recovery{--stat-color:var(--amber)}.stat-action{position:absolute;right:12px;bottom:12px;z-index:2;min-width:34px;min-height:30px;padding:4px 9px!important;border-color:rgba(255,179,31,.4)!important;color:#ffd77a!important;background:rgba(81,51,4,.44)!important;font-size:11px!important}.panel{padding:22px;border:1px solid var(--border);border-radius:var(--radius);color:var(--text);background:linear-gradient(145deg,rgba(9,25,44,.96),rgba(5,17,32,.96));box-shadow:var(--shadow),inset 0 1px 0 rgba(255,255,255,.025)}.panel h2{margin-top:0;font-size:18px;letter-spacing:-.01em}.page-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(310px,1fr));gap:18px;align-items:start}.posts-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(270px,1fr));gap:15px;align-items:stretch}.post-card{--status:var(--slate);position:relative;display:flex;flex-direction:column;min-height:287px;padding:20px;overflow:hidden;border:1px solid color-mix(in srgb,var(--status) 47%,#20344a);border-top:3px solid var(--status);border-radius:11px;color:var(--text);background:linear-gradient(150deg,color-mix(in srgb,var(--status) 8%,#071426),#06111f 72%);box-shadow:0 18px 38px rgba(0,0,0,.24),0 0 28px color-mix(in srgb,var(--status) 5%,transparent),inset 0 1px 0 rgba(255,255,255,.025);transition:transform .18s ease,border-color .18s ease,box-shadow .18s ease;}.post-card::after{content:"";position:absolute;left:15%;right:15%;top:-5px;height:4px;border-radius:50%;background:var(--status);opacity:.3;filter:blur(8px)}.post-card:hover{transform:translateY(-2px);border-color:color-mix(in srgb,var(--status) 66%,#243b51);box-shadow:0 22px 46px rgba(0,0,0,.3),0 0 34px color-mix(in srgb,var(--status) 7%,transparent)}.post-card.active{--status:var(--green)}.post-card.idle,.post-card.unknown{--status:var(--slate)}.post-card.offline{--status:var(--red)}.post-card.error{--status:#ff7d45}.post-card.recovery_pending{--status:var(--amber)}.post-header{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:9px;padding-bottom:12px;border-bottom:1px solid rgba(62,89,115,.38)}.post-heading{display:flex;align-items:center;gap:10px;min-width:0;flex:1}.playstation-mark{display:grid;place-items:center;flex:0 0 38px;width:38px;height:38px;border:1px solid #1b67e8;border-radius:10px;color:#fff;background:linear-gradient(145deg,#155fdb,#071f59);box-shadow:0 7px 18px rgba(8,75,211,.25),inset 0 1px 0 rgba(255,255,255,.16)}.playstation-mark svg{width:27px;height:27px;fill:currentColor;filter:drop-shadow(0 1px 2px rgba(0,0,0,.42))}.post-title{min-width:0;margin:0;overflow-wrap:anywhere;color:#f7f9ff;font-size:18px;font-weight:750;line-height:1.22;letter-spacing:-.02em}.meta{margin:3px 0;color:#8197b0;font-size:11px;line-height:1.45;word-break:break-word}.badge{display:inline-flex;align-items:center;gap:7px;padding:6px 9px;border:1px solid color-mix(in srgb,var(--badge-color) 35%,#253b51);border-radius:999px;color:var(--badge-color);background:color-mix(in srgb,var(--badge-color) 8%,#081523);font-size:10px;font-weight:750;white-space:nowrap}.badge::before{content:"";width:6px;height:6px;border-radius:50%;background:currentColor;box-shadow:0 0 8px currentColor}.badge.active{--badge-color:var(--green)}.badge.idle,.badge.unknown{--badge-color:var(--slate)}.badge.offline{--badge-color:var(--red)}.badge.error{--badge-color:#ff7d45}.badge.recovery_pending{--badge-color:var(--amber)}.session-row{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:13px;align-items:stretch;margin:9px 0 13px}.timer{display:flex;flex-direction:column;justify-content:center;min-width:0;padding:10px 0}.timer-label{margin-bottom:7px;color:#91a4bb;font-size:11px}.timer-value{overflow:hidden;color:var(--status);font-family:"SFMono-Regular",Consolas,"Liberation Mono",monospace;font-size:clamp(31px,2.4vw,39px);font-weight:750;line-height:1;letter-spacing:-.07em;text-overflow:clip;text-shadow:0 0 15px color-mix(in srgb,var(--status) 34%,transparent);font-variant-numeric:tabular-nums;white-space:nowrap}.coin-controls{display:grid;grid-template-columns:repeat(2,39px);gap:6px;align-content:end;padding-left:12px;border-left:1px solid rgba(61,88,114,.45)}.coin-label{grid-column:1/-1;color:#7c91a9;font-size:9px;text-align:center;text-transform:uppercase;letter-spacing:.12em}.relay{margin:auto 0 10px;color:#8da1b8;font-size:11px;font-weight:700}.relay.on{color:var(--green)}.relay.off{color:var(--red)}.recovery-box{display:flex;flex:1;flex-direction:column;justify-content:center;margin:8px 0 14px;padding:14px;border:1px solid rgba(255,179,31,.18);border-radius:9px;color:#ffd276;background:rgba(73,47,5,.18)}.recovery-copy{display:flex;align-items:center;gap:11px;font-size:13px;line-height:1.35}.recovery-copy svg{flex:0 0 auto;width:34px;height:34px;color:var(--amber);filter:drop-shadow(0 0 7px rgba(255,179,31,.3))}.recovery-time{margin-top:9px;color:#f5b93d;font-family:"SFMono-Regular",Consolas,monospace;font-size:25px;font-weight:750;font-variant-numeric:tabular-nums}.offline-state{display:flex;flex:1;flex-direction:column;align-items:center;justify-content:center;gap:8px;min-height:115px;color:var(--red)}.offline-state svg{width:45px;height:45px;filter:drop-shadow(0 0 9px rgba(255,85,99,.27))}.offline-state span{font-size:12px;font-weight:700}.post-card .actions{gap:7px;margin-top:auto}.post-card button.action{min-height:39px;padding:8px 12px;border-radius:7px;font-size:11px}.post-card .coin-action{min-width:39px;min-height:37px;padding:7px;border-color:rgba(0,217,255,.42);color:#dffaff;background:rgba(0,111,150,.18)}.actions{display:flex;gap:8px;flex-wrap:wrap}button.action{min-height:41px;padding:9px 14px;border:1px solid transparent;border-radius:8px;font-size:12px;font-weight:750;cursor:pointer;transition:filter .16s ease,transform .16s ease,background .16s ease,border-color .16s ease}button.action:hover:not(:disabled){filter:brightness(1.15);transform:translateY(-1px)}button.action:disabled{opacity:.32;cursor:not-allowed;filter:saturate(.35)}.primary{color:#e8fbff;background:linear-gradient(135deg,#0877ff,#075fcf);box-shadow:0 8px 20px rgba(8,119,255,.16)}.secondary{color:#d5e7f7;border-color:#365470!important;background:#0d2136}.warn{color:#ffe2a4;border-color:rgba(255,179,31,.42)!important;background:rgba(105,65,4,.42)}.danger{color:#ffe7e9;border-color:rgba(255,85,99,.48)!important;background:rgba(135,24,39,.55)}.form-group{display:flex;flex-direction:column;gap:6px;margin-bottom:12px}.form-group label{color:#aebed0;font-size:12px;font-weight:650}input,textarea,select{width:100%;padding:11px 12px;border:1px solid #294159;border-radius:8px;color:#eef6ff;background:#071628;font-size:13px;transition:border-color .16s ease,box-shadow .16s ease}input:focus,textarea:focus,select:focus{border-color:#1575b2;box-shadow:0 0 0 3px rgba(0,161,255,.1)}input::placeholder,textarea::placeholder{color:#5f748c}textarea{min-height:220px;resize:vertical;font-family:monospace}.message{margin-top:10px;font-size:13px;color:var(--muted);min-height:18px}.pending-list{display:grid;gap:10px}.pending-item{padding:14px;border:1px solid #243b51;border-radius:10px;background:rgba(7,22,39,.82)}.editor{display:none;margin-top:12px;padding-top:12px;border-top:1px solid #263e55}.logs{height:62vh;min-height:360px;padding:14px;overflow:auto;border:1px solid #1c354d;border-radius:10px;color:#a9c7df;background:#020b16;font-family:"SFMono-Regular",Consolas,monospace;font-size:11px}.log-item{padding:7px 0;border-bottom:1px solid rgba(255,255,255,.08)}.empty{padding:20px;border:1px dashed #31506b;border-radius:10px;color:#7890a9;background:rgba(5,21,37,.56)}code{color:#7deaff}@media (max-width:1260px){.stats{grid-template-columns:repeat(3,minmax(0,1fr))}}@media (max-width:980px){.app-shell{display:block}.sidebar{position:fixed;left:0;top:0;width:min(82vw,280px);transform:translateX(-102%);transition:transform .22s ease}body.sidebar-open .sidebar{transform:translateX(0)}body.sidebar-open .sidebar-overlay{display:block}.mobile-menu{display:grid;place-items:center;flex:0 0 auto}.topbar-title{margin-right:auto}.content{padding:24px}}@media (max-width:680px){.topbar{min-height:72px;padding:11px 14px;gap:11px}.eyebrow,.live-pill{display:none}.topbar-title h1{font-size:20px}.clock{padding:0 9px}.content{padding:20px 14px 30px}.stats{grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.stat-card{min-height:113px;padding:13px}.stat-top{gap:8px;font-size:10px}.stat-icon{width:32px;height:32px}.stat-icon svg{width:18px;height:18px}.stat-value{font-size:26px}.stat-card.duration{grid-column:span 2}.section-heading{margin-top:29px}.posts-grid,.page-grid{grid-template-columns:1fr}.post-card{min-height:275px;padding:17px}.panel{padding:17px}}@media (max-width:390px){.clock{display:none}.stats{grid-template-columns:1fr}.stat-card.duration{grid-column:auto}.stat-card{min-height:104px}.posts-list-title{font-size:19px}}</style></head><body><div class="app-shell"><aside class="sidebar" id="appSidebar"><div class="brand-wrap"><img class="brand-logo" src="/assets/game-room-logo-v4.webp" alt="PS TIME MANAGER" /><p class="brand-caption">Gestion du temps de jeu</p></div><nav class="nav" aria-label="Navigation principale"><p class="nav-label">Console</p><a href="/" data-path="/"><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8M12 17v4"/></svg><span>Vue des postes</span></a><a href="/config" data-path="/config" data-admin-only hidden><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .34 1.88l.06.06-2.83 2.83-.06-.06a1.7 1.7 0 0 0-1.88-.34 1.7 1.7 0 0 0-1.03 1.56V21h-4v-.09A1.7 1.7 0 0 0 8.94 19.4a1.7 1.7 0 0 0-1.88.34l-.06.06-2.83-2.83.06-.06A1.7 1.7 0 0 0 4.57 15 1.7 1.7 0 0 0 3 14H3v-4h.09A1.7 1.7 0 0 0 4.6 8.94a1.7 1.7 0 0 0-.34-1.88L4.2 7l2.83-2.83.06.06A1.7 1.7 0 0 0 9 4.57 1.7 1.7 0 0 0 10 3.09V3h4v.09A1.7 1.7 0 0 0 15.06 4.6a1.7 1.7 0 0 0 1.88-.34L17 4.2 19.83 7l-.06.06A1.7 1.7 0 0 0 19.43 9 1.7 1.7 0 0 0 20.91 10H21v4h-.09A1.7 1.7 0 0 0 19.4 15Z"/></svg><span>Configuration</span></a><a href="/logs" data-path="/logs" data-admin-only hidden><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M6 3h12a2 2 0 0 1 2 2v16H4V5a2 2 0 0 1 2-2Z"/><path d="M8 8h8M8 12h8M8 16h5"/></svg><span>Journal</span></a><a href="/postes" data-path="/postes" data-admin-only hidden><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/></svg><span>Gestion des postes</span></a><a href="/users" data-path="/users" data-admin-only hidden><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M22 21v-2a4 4 0 0 0-3-3.87M16 3.13a4 4 0 0 1 0 7.75"/></svg><span>Utilisateurs</span></a><a href="/security" data-path="/security" data-admin-only hidden><svg class="nav-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10Z"/><path d="m9 12 2 2 4-4"/></svg><span>Sécurité</span></a></nav><div class="sidebar-footer"><div class="user-panel"><div class="user-avatar" id="currentUserAvatar">PT</div><div class="user-copy"><div class="user-name" id="currentUserLabel">Compte local</div><div class="user-role" id="currentUserRole">Connexion sécurisée</div></div></div><button class="sidebar-action danger-btn" data-admin-only hidden onclick="resetWifi()"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M5 12.55a11 11 0 0 1 14.08 0M8.53 16.11a6 6 0 0 1 6.95 0M12 20h.01M2 3l20 18"/></svg>
Réinitialiser le Wi-Fi
</button><button class="sidebar-action" onclick="logout()"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4M16 17l5-5-5-5M21 12H9"/></svg>
Déconnexion
</button></div></aside><div class="workspace"><header class="topbar"><button class="mobile-menu" type="button" aria-label="Ouvrir le menu" aria-controls="appSidebar" aria-expanded="false" onclick="toggleSidebar()"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true"><path d="M4 6h16M4 12h16M4 18h16"/></svg></button><div class="topbar-title"><div class="eyebrow">Console centrale</div><h1 id="pageHeading">Vue des postes</h1></div><div class="topbar-meta"><div class="live-pill"><span class="live-dot"></span>En ligne</div><div class="clock"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></svg><span id="currentClock">--:--:--</span></div></div></header><main class="content"><section class="view" id="view-home"><h2 class="page-title">Vue des postes</h2><div class="stats" id="stats"></div><div class="section-heading"><div><h2 class="posts-list-title">Liste des postes</h2><p class="section-kicker">État et contrôle des stations en temps réel</p></div></div><div id="posts" class="posts-grid"></div></section><section class="view" id="view-config"><h2 class="page-title">Configuration et import/export</h2><div class="page-grid"><div class="panel"><h2>Configuration générale</h2><div class="form-group"><label for="coinDurationMinutes">Durée par coin (minutes)</label><input id="coinDurationMinutes" type="number" min="1" step="1" placeholder="Durée par coin (min)" /></div><div class="form-group"><label for="pulsesPerCoin">Impulsions par coin</label><input id="pulsesPerCoin" type="number" min="1" placeholder="Impulsions par coin" /></div><div class="form-group"><label for="availableCoins">Coins disponibles</label><input id="availableCoins" type="number" min="0" placeholder="Crédit disponible" /></div><button class="action primary" onclick="saveConfig()">Enregistrer</button><div class="message" id="configMessage"></div></div><div class="panel"><h2>Export / Import</h2><div class="actions"><button class="action primary" onclick="exportConfig()">Exporter JSON</button><button class="action secondary" onclick="importConfig()">Importer JSON</button></div><div class="form-group" style="margin-top:12px;"><textarea id="configJson" placeholder="Colle ici le JSON d'export/import"></textarea></div><div class="message" id="importExportMessage"></div></div></div></section><section class="view" id="view-logs"><h2 class="page-title">Logs</h2><div class="panel"><div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;"><h2 style="margin:0;">Journal d'événements</h2><div class="actions"><button class="action primary" onclick="loadLogs()">Actualiser</button><button class="action danger" onclick="clearLogs()">Vider</button></div></div><div id="logs" class="logs"></div></div></section><section class="view" id="view-postes"><h2 class="page-title">Gestion des postes</h2><div class="page-grid"><div class="panel"><div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;"><h2 style="margin:0;">Postes configurés</h2><button class="action primary" onclick="load()">Actualiser</button></div><div id="managedPosts" class="pending-list"></div><div class="message" id="managedMessage"></div></div><div class="panel"><h2>Postes découverts</h2><div id="pendingPosts" class="pending-list"></div><div class="message" id="pendingMessage"></div></div></div></section><section class="view" id="view-users"><h2 class="page-title">Gestion des utilisateurs</h2><div class="page-grid"><div class="panel"><h2>Ajouter un utilisateur</h2><div class="form-group"><label for="createUsername">Nom d'utilisateur</label><input id="createUsername" maxlength="32" placeholder="Nom d'utilisateur unique" /></div><div class="form-group"><label for="createFirstName">Prénom</label><input id="createFirstName" maxlength="64" placeholder="Prénom" /></div><div class="form-group"><label for="createLastName">Nom</label><input id="createLastName" maxlength="64" placeholder="Nom" /></div><div class="form-group"><label for="createPassword">Mot de passe</label><input id="createPassword" type="password" minlength="6" placeholder="6 caractères minimum" /></div><div class="form-group"><label for="createRole">Rôle</label><select id="createRole"><option value="user">Utilisateur simple</option><option value="admin">Administrateur</option></select></div><button class="action primary" onclick="createUser()">Ajouter</button><div class="message" id="createUserMessage"></div></div><div class="panel"><div style="display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:12px;"><h2 style="margin:0;">Comptes existants</h2><button class="action primary" onclick="loadUsers()">Actualiser</button></div><div id="usersList" class="pending-list"></div><div class="message" id="usersMessage"></div></div></div></section><section class="view" id="view-security"><h2 class="page-title">Mot de passe et API token</h2><div class="page-grid"><div class="panel"><h2>Votre mot de passe administrateur</h2><div class="form-group"><input id="newPassword" type="password" minlength="6" placeholder="Nouveau mot de passe (6 caractères minimum)" /></div><button class="action primary" onclick="changePassword()">Changer mot de passe</button><div class="message" id="authMessage"></div></div><div class="panel"><h2>Token API</h2><div id="authInfo" style="font-size:13px;color:#8fa3bd;"></div><button class="action warn" style="margin-top:14px;" onclick="regenerateToken()">Régénérer token API</button><div class="message" id="tokenMessage"></div></div></div></section></main></div></div><div class="sidebar-overlay" aria-hidden="true" onclick="closeSidebar()"></div>

<script>
const navigationOrder = ['/', '/postes', '/users', '/security', '/logs', '/config'];
const primaryNavigation = document.querySelector('.nav');
navigationOrder.forEach(path => {
  const link = primaryNavigation.querySelector(`[data-path="${path}"]`);
  if (link) primaryNavigation.appendChild(link);
});

const viewsByPath = {
'/':'view-home',
'/config':'view-config',
'/logs':'view-logs',
'/discover':'view-postes',
'/postes':'view-postes',
'/users':'view-users',
'/security':'view-security'
};
const pageTitles = {
'/':'Vue des postes',
'/config':'Configuration',
'/logs':'Journal d’événements',
'/discover':'Gestion des postes',
'/postes':'Gestion des postes',
'/users':'Gestion des utilisateurs',
'/security':'Sécurité et accès API'
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
const fullName = identity ? `${identity.firstName || ''} ${identity.lastName || ''}`.trim() : '';
document.getElementById('currentUserLabel').textContent = fullName || identity?.username || 'Accès API';
document.getElementById('currentUserRole').textContent = identity?.role === 'admin' ? 'Administrateur' : 'Utilisateur simple';
document.getElementById('currentUserAvatar').textContent = identity
? `${identity.firstName?.[0] || identity.username?.[0] || ''}${identity.lastName?.[0] || ''}`.toUpperCase()
: 'API';
if (!isAdmin && currentPath !== '/') {
startPage('/');
}
}
function activateView() {
const navPath = currentPath === '/discover' ? '/postes' : currentPath;
document.querySelectorAll('.view').forEach(view => view.classList.remove('active'));
document.getElementById(viewsByPath[currentPath]).classList.add('active');
document.querySelectorAll('.nav a').forEach(link => link.classList.toggle('active', link.dataset.path === navPath));
document.getElementById('pageHeading').textContent = pageTitles[currentPath] || 'PS TIME MANAGER';
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
closeSidebar();
});
});
window.addEventListener('popstate', () => {
startPage(window.location.pathname, false);
});
}
function toggleSidebar() {
const open = document.body.classList.toggle('sidebar-open');
document.querySelector('.mobile-menu')?.setAttribute('aria-expanded', String(open));
}
function closeSidebar() {
document.body.classList.remove('sidebar-open');
document.querySelector('.mobile-menu')?.setAttribute('aria-expanded', 'false');
}
function updateClock() {
const clock = document.getElementById('currentClock');
if (clock) clock.textContent = new Date().toLocaleTimeString('fr-FR', { hour12:false });
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
el.style.color = isError ? '#ff7a86' : '#8fa3bd';
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
<div class="stat-card coins">
<div class="stat-top">
<span class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><ellipse cx="12" cy="6" rx="7" ry="3"/><path d="M5 6v5c0 1.66 3.13 3 7 3s7-1.34 7-3V6M5 11v5c0 1.66 3.13 3 7 3s7-1.34 7-3v-5"/></svg></span>
<span>Coins disponibles</span>
</div>
<div class="stat-value">${data.availableCoins}</div>
${currentRole === 'admin' ? '<button class="action stat-action" title="Simuler un coin" aria-label="Simuler un coin" onclick="simulateCoin()">+1</button>' : ''}
</div>
<div class="stat-card duration">
<div class="stat-top"><span class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><circle cx="12" cy="12" r="8"/><path d="M12 7v5l3 2"/></svg></span><span>Durée par coin</span></div>
<div class="stat-value">${formatTime(data.coinDurationSeconds)}</div>
</div>
<div class="stat-card online">
<div class="stat-top"><span class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M16 19v-1.5A3.5 3.5 0 0 0 12.5 14h-5A3.5 3.5 0 0 0 4 17.5V19"/><circle cx="10" cy="7.5" r="3.5"/><path d="m17 10 2 2 3-4"/></svg></span><span>Postes actifs</span></div>
<div class="stat-value">${activeCount}</div>
</div>
<div class="stat-card disconnected">
<div class="stat-top"><span class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M12 2v10M6.34 5.34a8 8 0 1 0 11.32 0"/></svg></span><span>Postes hors ligne</span></div>
<div class="stat-value">${offlineCount}</div>
</div>
<div class="stat-card recovery">
<div class="stat-top"><span class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M4 7v5h5M20 17v-5h-5"/><path d="M6.1 16A7 7 0 0 0 18 18M17.9 8A7 7 0 0 0 6 6"/></svg></span><span>Reprises en attente</span></div>
<div class="stat-value">${recoveryCount}</div>
</div>
`;
document.getElementById('coinDurationMinutes').value = Number(data.coinDurationSeconds || 1800) / 60;
document.getElementById('pulsesPerCoin').value = data.pulsesPerCoin || 1;
document.getElementById('availableCoins').value = data.availableCoins || 0;
document.getElementById('authInfo').innerHTML = `
<b>Token API :</b> <code>${data.apiTokenMasked || ''}</code><br>
<span style="font-size:11px;color:#7087a1;">Header: Authorization: Bearer TON_TOKEN</span>
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
${p.recoveryPending ? `
<div class="recovery-box">
<div class="recovery-copy">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M4 7v5h5M20 17v-5h-5"/><path d="M6.1 16A7 7 0 0 0 18 18M17.9 8A7 7 0 0 0 6 6"/></svg>
<b>Coupure détectée</b>
</div>
<div class="recovery-time">${formatTime(p.recoveryRemaining)}</div>
</div>
<div class="relay off">Relais : OFF</div>
<div class="actions">
<button class="action primary" onclick="resumeRecovery('${esc(p.id)}')">Reprendre</button>
<button class="action warn" onclick="cancelRecovery('${esc(p.id)}')">Annuler</button>
</div>
` : p.status === 'offline' ? `
<div class="offline-state">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.55" aria-hidden="true"><path d="M12 2v10M6.34 5.34a8 8 0 1 0 11.32 0"/></svg>
<span>Poste indisponible</span>
</div>
<div class="relay off">Relais : OFF</div>
<div class="actions">
<button class="action secondary" disabled>Arrêter</button>
</div>
` : `
<div class="session-row" style="grid-template-columns:minmax(0,1fr);">
<div class="timer">
<div class="timer-label">Temps restant</div>
<div class="timer-value">${formatTime(p.remaining)}</div>
</div>
</div>
<div class="relay ${p.relay ? 'on' : 'off'}">Relais : ${p.relay ? 'ON' : 'OFF'}</div>
<div class="actions">
<button class="action danger" title="Arrêter ce poste" ${(p.status !== 'active' && Number(p.remaining || 0) <= 0) ? 'disabled' : ''} onclick="stopPost('${esc(p.id)}')">Arrêter la session</button>
<div class="coin-controls" style="margin-left:auto;">
<div class="coin-label">Coins</div>
<button class="action coin-action" title="Affecter 1 coin de plus pour ce poste" aria-label="Affecter 1 coin de plus pour ce poste" onclick="assign('${esc(p.id)}',1)">+1</button>
<button class="action coin-action" title="Affecter 2 coins de plus pour ce poste" aria-label="Affecter 2 coins de plus pour ce poste" onclick="assign('${esc(p.id)}',2)">+2</button>
</div>
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
const yes = confirm(`Arrêter la session de ${postLabel(postId)} ?\nLe temps restant sera définitivement perdu.`);
if (!yes) return;
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
updateClock();
setInterval(updateClock, 1000);
window.addEventListener('resize', () => { if (window.innerWidth > 980) closeSidebar(); });
setupNavigation();
startPage(currentPath, false);
</script>
</body></html>
)rawliteral";
  }
}
