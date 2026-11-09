"""Publish the tested SD archive using the workflow token."""
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

repo = os.environ['GITHUB_REPOSITORY']
sha = os.environ['GITHUB_SHA']
token = os.environ['GH_TOKEN']
headers = {'Authorization': 'Bearer '+token, 'Accept': 'application/vnd.github+json', 'User-Agent': 'PhonePadNX-release', 'X-GitHub-Api-Version': '2022-11-28'}
base = 'https://api.github.com/repos/'+repo
asset = Path('dist/PhonePadNX-0.1.0-test.zip')
body = """Первая версия для проверки на Switch.

Телефон подключается через браузер по Wi-Fi. Есть кнопки Pro Controller, два стика и подключение по PIN.

Распакуй ZIP в корень SD, перезагрузи консоль и открой PhonePadNX. Дальше отсканируй QR телефоном.

Сборка и автоматические тесты прошли. На настоящей консоли ещё не проверено.
"""
payload = {'tag_name':'v0.1.0-test', 'target_commitish':sha, 'name':'0.1.0 — тестовая версия', 'body':body, 'draft':False, 'prerelease':True, 'generate_release_notes':False}
request = urllib.request.Request(base+'/releases', data=json.dumps(payload).encode(), headers={**headers,'Content-Type':'application/json'}, method='POST')
with urllib.request.urlopen(request, timeout=30) as response:
    release=json.load(response)
upload=release['upload_url'].split('{',1)[0]+'?name='+urllib.parse.quote(asset.name)
request=urllib.request.Request(upload,data=asset.read_bytes(),headers={**headers,'Content-Type':'application/zip'},method='POST')
with urllib.request.urlopen(request, timeout=60) as response:
    result=json.load(response)
print(result['browser_download_url'])
