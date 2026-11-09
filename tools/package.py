from pathlib import Path
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
sd = root / 'dist/SD'
module = sd / 'atmosphere/contents/4200000000505044'
app = sd / 'switch/PhonePadNX'
for p in [module / 'flags', app]:
    p.mkdir(parents=True, exist_ok=True)
shutil.copy2(root / 'sysmodule/phonepad-sys.nsp', module / 'exefs.nsp')
shutil.copy2(root / 'launcher/PhonePadNX.nro', app / 'PhonePadNX.nro')
(module / 'flags/boot2.flag').touch()
(module / 'toolbox.json').write_text('{"name":"PhonePadNX","tid":"4200000000505044","requires_reboot":true}\n')
shutil.copy2(root / 'README.md', root / 'dist/README.md')
with zipfile.ZipFile(root / 'dist/PhonePadNX-0.1-experimental.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for path in sorted(sd.rglob('*')):
        if path.is_file():
            z.write(path, path.relative_to(sd))
    z.write(root / 'README.md', 'README.md')
print('Created dist/PhonePadNX-0.1-experimental.zip')
