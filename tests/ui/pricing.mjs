// Exercise the embedded HTML against a mocked API. Firmware services are tested separately in tests/host.
import fs from 'node:fs/promises';
import path from 'node:path';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const { chromium } = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const source = await fs.readFile(path.join(root, 'central-esp32/WebPage.cpp'), 'utf8');
const html = [...source.matchAll(/R"rawliteral\(([\s\S]*?)\)rawliteral"/g)][1][1];
const browser = await chromium.launch({ headless: true, ...(process.env.CHROMIUM_EXECUTABLE_PATH ? {executablePath:process.env.CHROMIUM_EXECUTABLE_PATH} : {}) });
const page = await browser.newPage({ viewport: { width: 1440, height: 1050 } });
const errors = [], writes = [];
page.on('pageerror', error => errors.push(error.message));
page.on('dialog', dialog => dialog.accept());
let role = 'admin', slowPosts = false;
const state = {
  posts: [1, 4, 1].map((price, i) => ({ id:`ps${i+1}`, chipId:`ps${i+1}`, name:`PS${i+1}`,
    ip:`192.168.1.${i+11}`, piecesRequired:price, status:'idle', relay:false, remaining:0 })),
  pendingPosts:[{chipId:'ps4',ip:'192.168.1.14'}], availablePieces:6, coinDurationSeconds:1800,
  creditUnit:'pulse', assignmentsReady:true, coinCurrency:'TND', pulseValueMinor:1000, dzdToTndRateMicros:0,
  creditStorageReady:true, assignmentRecoveryPending:false, revenueStorageReady:true, revenueTimeReady:true,
};
await page.route('http://gameroom.test/**', async route => {
  const request = route.request(), url = new URL(request.url()), endpoint = url.pathname;
  const json = async data => route.fulfill({ contentType:'application/json', body:JSON.stringify(data) });
  if (request.method() === 'POST') {
    const body = request.postDataJSON?.() || {};
    writes.push({ endpoint, body });
    if (endpoint === '/post/update') Object.assign(state.posts.find(post => post.id === body.id), body);
    if (endpoint === '/poste/configure') {
      state.posts.push({ id:body.chipId, ...body, ip:'192.168.1.14', status:'idle', remaining:0 });
      state.pendingPosts = [];
    }
    if (endpoint === '/assign') {
      const post = state.posts.find(post => post.id === body.post_id);
      state.availablePieces -= post.piecesRequired; post.remaining += state.coinDurationSeconds; post.status='active'; post.relay=true;
    }
    return json({ ok:true });
  }
  if (endpoint === '/posts') {
    if (slowPosts) await new Promise(resolve => setTimeout(resolve, 250));
    return json({ ...state, accessRole:role, currentUser:{username:role,firstName:'Test',lastName:'Compte',role} });
  }
  if (endpoint.startsWith('/assets/')) {
    const files = {'login-station-v4.webp':'login-station-embedded.webp','game-room-logo-v5.webp':'game-room-logo-embedded.webp','playstation-mark-v2.webp':'playstation-mark.webp'};
    const filename = files[path.basename(endpoint)];
    return filename ? route.fulfill({contentType:'image/webp', body:await fs.readFile(path.join(root,'central-esp32/ui-assets',filename))}) : route.fulfill({status:404});
  }
  if (endpoint === '/logs/data') return json({ logs:[] });
  return route.fulfill({ contentType:'text/html', body:html });
});
try {
  await page.goto('http://gameroom.test/');
  await page.locator('.piece-assign').first().waitFor();
  assert.deepEqual(await page.locator('.piece-assign').allTextContents(), ['+1','+4','+1']);
  await page.getByRole('button', {name:'Ajouter une durée de jeu pour 4 pièces', exact:true}).click();
  await page.waitForFunction(() => document.querySelector('.stat-card.coins .stat-value')?.textContent === '2');
  assert.deepEqual(writes.at(-1),{endpoint:'/assign',body:{post_id:'ps2'}});
  assert.equal(await page.getByRole('button',{name:'Ajouter une durée de jeu pour 4 pièces',exact:true}).isDisabled(),true);
  assert.match(await page.locator('.post-card').nth(1).innerText(),/30:00/);
  const homeAligned = await page.locator('.post-actions').evaluateAll(rows => rows.every(row => {
    const buttons = [...row.querySelectorAll('button')].map(button => button.getBoundingClientRect());
    return buttons.every(button => Math.abs(button.top - buttons[0].top) < 1);
  }));
  assert.equal(homeAligned,true);
  await page.screenshot({path:'/private/tmp/gameroom-pricing-home.png',fullPage:true,animations:'disabled'});
  slowPosts = true;
  const refresh = page.waitForResponse(response => response.url().endsWith('/posts'));
  await page.locator('.nav a[data-path="/postes"]').click();
  const listAligned = await page.locator('.managed-post').evaluateAll(rows => rows.every(row => {
    const buttons = [...row.querySelectorAll('.managed-actions button')].map(button => button.getBoundingClientRect());
    const badge = row.querySelector('.badge').getBoundingClientRect();
    const price = row.querySelector('.managed-price');
    return buttons.every(button => Math.abs(button.top - buttons[0].top) < 1)
      && buttons[0].left - badge.right <= 13 && price.scrollWidth <= price.clientWidth
      && row.scrollWidth <= row.clientWidth;
  }));
  assert.equal(listAligned,true);
  await page.screenshot({path:'/private/tmp/gameroom-pricing-posts-desktop.png',fullPage:true,animations:'disabled'});
  await page.locator('#create-name-ps4').fill('PS4');
  await page.locator('#create-pieces-ps4').fill('3');
  await refresh;
  await page.evaluate(() => new Promise(requestAnimationFrame));
  assert.equal(await page.locator('#create-name-ps4').inputValue(),'PS4');
  assert.equal(await page.locator('#create-pieces-ps4').inputValue(),'3');
  slowPosts = false;
  await page.getByRole('button',{name:'Ajouter ce poste'}).click();
  await page.locator('#editor-ps4').waitFor({state:'attached'});
  assert.deepEqual(writes.at(-1).body,{chipId:'ps4',name:'PS4',piecesRequired:3});
  await page.locator('.managed-post').filter({has:page.locator('#editor-ps2')}).getByRole('button',{name:'Modifier',exact:true}).click();
  assert.equal(await page.locator('#edit-name-ps2').isDisabled(),true);
  await page.locator('#edit-pieces-ps2').fill('2');
  await page.locator('#editor-ps2').getByRole('button',{name:'Enregistrer modification'}).click();
  await page.waitForFunction(() => document.querySelector('#edit-pieces-ps2')?.value === '2');
  assert.deepEqual(writes.at(-1).body,{id:'ps2',name:'PS2',piecesRequired:2});
  assert.equal(state.posts[1].remaining,1800);
  await page.locator('.nav a[data-path="/"]').click();
  await page.getByRole('button',{name:'Ajouter une durée de jeu pour 2 pièces',exact:true}).waitFor();
  assert.equal(await page.getByRole('button',{name:'Ajouter une durée de jeu pour 2 pièces',exact:true}).isEnabled(),true);
  await page.locator('.nav a[data-path="/config"]').click();
  assert.equal(await page.locator('#pulsesPerCoin').count(),0);
  await page.getByLabel('Pièces disponibles').waitFor();
  await page.screenshot({path:'/private/tmp/gameroom-pricing-config.png',fullPage:true,animations:'disabled'});
  // Vérifier aussi les formulaires et les cartes sur un écran étroit.
  await page.setViewportSize({width:390,height:844});
  await page.locator('.nav a[data-path="/postes"]').evaluate(link => link.click());
  await page.screenshot({path:'/private/tmp/gameroom-pricing-posts-mobile.png',fullPage:true,animations:'disabled'});
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),true);
  role='user';
  await page.goto('http://gameroom.test/');
  await page.locator('.piece-assign').first().waitFor();
  assert.equal(await page.locator('.nav a[data-path="/postes"]').isVisible(),false);
  assert.equal(await page.locator('.piece-assign').count(),4);
  await page.screenshot({path:'/private/tmp/gameroom-pricing-home-mobile.png',fullPage:true,animations:'disabled'});
  assert.deepEqual(errors,[]);
  console.log('PASS: embedded UI pricing buttons, debit request, insufficient balance, create/edit, active tariff, config, mobile and user permissions');
} catch (error) {
  console.error('UI diagnostics:', JSON.stringify({errors,writes,pending:await page.locator('#pendingMessage').textContent()}));
  await page.screenshot({path:'/private/tmp/gameroom-pricing-failure.png',fullPage:true,animations:'disabled'});
  throw error;
} finally { await browser.close(); }
