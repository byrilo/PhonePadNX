from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
archive = root / 'dist/PhonePadNX-0.3.0-test.zip'
archive.parent.mkdir(exist_ok=True)
module = 'atmosphere/contents/4200000000505044'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    z.write(root / 'sysmodule/phonepad-sys.nsp', module + '/exefs.nsp')
    z.writestr(module + '/flags/boot2.flag', b'')
    z.write(root / 'launcher/PhonePadNX.nro', 'switch/PhonePadNX/PhonePadNX.nro')
    z.write(root / 'README.md', 'README.md')
