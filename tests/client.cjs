const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const { webcrypto } = require('node:crypto');
const html = fs.readFileSync(require('node:path').join(__dirname, '../web/index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
function harness(fetcher) {
  const elements = new Map();
  const events = {};
  const get = id => {
    if (!elements.has(id)) elements.set(id, { value: '', hidden: false, textContent: '' });
    return elements.get(id);
  };
  let now = 100;
  const document = { hidden: false, getElementById: get, querySelectorAll: () => [], addEventListener: (e, f) => events[e] = f };
  const sandbox = { document, window: { addEventListener: (e, f) => events[e] = f }, crypto: webcrypto,
    URLSearchParams, AbortController, TypeError, setTimeout, clearTimeout, setInterval: () => 1,
    location: { search: '?pin=123456', pathname: '/' }, history: { replaceState() {} },
    performance: { now: () => now }, fetch: fetcher };
  vm.createContext(sandbox); vm.runInContext(script, sandbox);
  return { get, events, document, run: code => vm.runInContext(code, sandbox), advance: ms => now += ms };
}
const response = (status = 200, text = 'OK') => ({ status, ok: status === 200, text: async () => text });
async function main() {
  let calls = 0;
  const good = harness(async () => { calls++; return response(); });
  await good.get('connect').onclick();
  assert.equal(calls, 2, 'claim and first input must both succeed before displaying controls');
  assert.equal(good.get('setup').hidden, true);
  good.run('sticks.l=[20000,-10000]; active.set(1,0)');
  good.events.blur();
  assert.equal(good.run('connected'), true, 'keyboard/window blur must not log out');
  assert.equal(good.run('buttons()'), 0);
  assert.equal(good.run('sticks.l[0]'), 0);

  let fail = false;
  const flaky = harness(async () => { if (fail) throw new TypeError('Failed to fetch'); return response(); });
  await flaky.get('connect').onclick();
  fail = true; await flaky.run('pump()');
  assert.equal(flaky.get('setup').hidden, true, 'one dropped packet must not show PIN form');
  fail = false; flaky.advance(200); await flaky.run('pump()');
  assert.equal(flaky.run('lostSince'), 0);
  flaky.document.hidden = true; flaky.events.visibilitychange();
  assert.equal(flaky.run('connected'), false, 'hiding the page still stops input');

  let phase = 0;
  const failedHandshake = harness(async () => response(++phase === 1 ? 200 : 503));
  await failedHandshake.get('connect').onclick();
  assert.equal(failedHandshake.get('setup').hidden, false, 'failed first input must never flash controls');
  assert.equal(failedHandshake.run('connected'), false);
  assert.match(failedHandshake.get('message').textContent, /Switch/);

  const slow = harness(async (_path, options) => {
    await new Promise((resolve, reject) => {
      const timer = setTimeout(resolve, 400);
      options.signal.addEventListener('abort', () => { clearTimeout(timer); reject(new DOMException('timeout', 'AbortError')); }, { once: true });
    });
    return response();
  });
  await slow.get('connect').onclick();
  assert.equal(slow.get('setup').hidden, true, '400ms replies must survive the handshake');
  console.log('Client regression tests passed: handshake, mobile blur, packet loss, recovery, visibility, slow replies.');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
