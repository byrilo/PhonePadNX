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
asset = Path('dist/PhonePadNX-0.3.3-test.zip')
body = """Добавлена кнопка HOME на телефоне. На Switch пока не проверена. Инструкция по запуску и отключению находится в ZIP.
"""

payload = {'tag_name':'v0.3.3-test', 'target_commitish':sha, 'name':'0.3.3 — игровой тест', 'body':body, 'draft':False, 'prerelease':True, 'generate_release_notes':False}
request = urllib.request.Request(base+'/releases', data=json.dumps(payload).encode(), headers={**headers,'Content-Type':'application/json'}, method='POST')
with urllib.request.urlopen(request, timeout=30) as response:
    release=json.load(response)
upload=release['upload_url'].split('{',1)[0]+'?name='+urllib.parse.quote(asset.name)
request=urllib.request.Request(upload,data=asset.read_bytes(),headers={**headers,'Content-Type':'application/zip'},method='POST')
with urllib.request.urlopen(request, timeout=60) as response:
    result=json.load(response)
print(result['browser_download_url'])
