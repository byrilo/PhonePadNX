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
asset = Path('dist/PhonePadNX-0.3.3.zip')
body = """Телефон как дополнительный геймпад для Switch по Wi-Fi. Есть два стика, кнопки Pro Controller и HOME.

На Switch с Ultra NX проверены подключение, нажатия в игре и переход в меню по HOME.

Скачай PhonePadNX-0.3.3.zip, распакуй в корень SD и перезагрузи консоль. Открой PhonePadNX, нажми A и отсканируй QR телефоном в той же Wi-Fi сети.
"""

payload = {'tag_name':'v0.3.3', 'target_commitish':sha, 'name':'PhonePadNX 0.3.3', 'body':body, 'draft':False, 'prerelease':False, 'make_latest':'true', 'generate_release_notes':False}
request = urllib.request.Request(base+'/releases', data=json.dumps(payload).encode(), headers={**headers,'Content-Type':'application/json'}, method='POST')
with urllib.request.urlopen(request, timeout=30) as response:
    release=json.load(response)
upload=release['upload_url'].split('{',1)[0]+'?name='+urllib.parse.quote(asset.name)
request=urllib.request.Request(upload,data=asset.read_bytes(),headers={**headers,'Content-Type':'application/zip'},method='POST')
with urllib.request.urlopen(request, timeout=60) as response:
    result=json.load(response)
print(result['browser_download_url'])
