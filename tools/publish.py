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
asset = Path('dist/PhonePadNX-0.2.0-diagnostic.zip')
body = """Только проверка связи с телефоном. Кнопки отображаются на экране Switch, управление играми отключено.

В архиве один NRO: нет фонового модуля, автозапуска и виртуального контроллера. Оставь приложение открытым во время теста.

Версии 0.1.0 и 0.1.1 отозваны после сообщения о перезагрузках и проблемах с Joy-Con. Старый модуль необходимо удалить из atmosphere/contents/4200000000505044 при выключенной консоли.

Это диагностическая сборка, причина перезагрузок ещё не установлена. На настоящей консоли не проверена.
"""
# Preserve previous releases and assets, but mark them as withdrawn.
for old_tag in ['v0.1.0-test','v0.1.1-test']:
    old_request=urllib.request.Request(base+'/releases/tags/'+old_tag,headers=headers)
    with urllib.request.urlopen(old_request,timeout=30) as response:
        old=json.load(response)
    warning='ОТОЗВАНО: не устанавливать. Есть сообщение о перезагрузках консоли и сбоях Joy-Con. Причина расследуется. Удалите старый модуль atmosphere/contents/4200000000505044 при выключенной консоли.\n\n'
    old_body=old.get('body') or ''
    if not old_body.startswith('ОТОЗВАНО:'):
        data=json.dumps({'name':old_tag+' — отозвано','body':warning+old_body,'prerelease':True}).encode()
        patch=urllib.request.Request(base+'/releases/'+str(old['id']),data=data,headers={**headers,'Content-Type':'application/json'},method='PATCH')
        with urllib.request.urlopen(patch,timeout=30) as response: response.read()

payload = {'tag_name':'v0.2.0-diagnostic', 'target_commitish':sha, 'name':'0.2.0 — проверка связи', 'body':body, 'draft':False, 'prerelease':True, 'generate_release_notes':False}
request = urllib.request.Request(base+'/releases', data=json.dumps(payload).encode(), headers={**headers,'Content-Type':'application/json'}, method='POST')
with urllib.request.urlopen(request, timeout=30) as response:
    release=json.load(response)
upload=release['upload_url'].split('{',1)[0]+'?name='+urllib.parse.quote(asset.name)
request=urllib.request.Request(upload,data=asset.read_bytes(),headers={**headers,'Content-Type':'application/zip'},method='POST')
with urllib.request.urlopen(request, timeout=60) as response:
    result=json.load(response)
print(result['browser_download_url'])
