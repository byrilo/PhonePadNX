"""Run from the repository root after compiling tests/simulator.c."""
import http.client
from pathlib import Path
import subprocess
import time
import urllib.request
import urllib.error

root = Path(__file__).resolve().parents[1]
proc = subprocess.Popen([str(root / 'tests/simulator.exe')], cwd=root, stdout=subprocess.DEVNULL)
base = 'http://127.0.0.1:8080'
owner = '0123456789abcdef0123456789abcdef'
def call(path, method='POST', header=True):
    request = urllib.request.Request(base+path, method=method, headers={'x-phonepad':'1'} if header else {})
    try:
        with urllib.request.urlopen(request, timeout=2) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as e:
        return e.code, e.read()
try:
    for attempt in range(30):
        try:
            code, page = call('/', 'GET'); break
        except OSError:
            time.sleep(.05)
    else:
        raise AssertionError('Server never started')
    assert code == 200 and b'PHONEPAD' in page
    claim = f'/api/claim?id={owner}&pin=123456'
    assert call(claim, 'GET')[0] == 403
    assert call(claim, header=False)[0] == 403
    assert call(claim)[0] == 200
    time.sleep(.32)
    assert call('/api/claim?id='+'f'*32+'&pin=123456')[0] == 409
    state = f'/api/state?id={owner}&seq=1&b=1&lx=100&ly=-200&rx=0&ry=0'
    assert call(state)[0] == 200
    assert (root/'simulator-state.txt').read_text() == '1 100 -200 0 0'
    assert call(state)[0] == 409
    # A partial HTTP header must not stop the input release watchdog.
    stalled = http.client.HTTPConnection('127.0.0.1',8080)
    stalled.connect(); stalled.sock.sendall(b'POST /api/state HTTP/1.1\r\n')
    time.sleep(.45)
    assert (root/'simulator-state.txt').read_text() == '0 0 0 0 0'
    assert call('/', 'GET')[0] == 200
    stalled.close()
    assert call('/'*2100,'GET')[0] == 413
    print('HTTP integration passed: page, auth, ownership, input, replay, slow client, watchdog, oversized request.')
finally:
    proc.terminate(); proc.wait(timeout=5)
    (root/'simulator-state.txt').unlink(missing_ok=True)
