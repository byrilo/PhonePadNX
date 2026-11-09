from pathlib import Path
import os
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
# Refuse to package code that still calls the retired virtual-controller backend.
compiler_root = Path(os.environ['DEVKITPRO']) / 'devkitA64/bin'
symbols = subprocess.check_output([str(compiler_root / 'aarch64-none-elf-nm'), str(root / 'launcher/PhonePadNX-check.elf')], text=True)
for name in ['hiddbgInitialize', 'hiddbgAttachHdlsWorkBuffer', 'hiddbgAttachHdlsVirtualDevice', 'hiddbgSetHdlsState']:
    if name in symbols:
        raise SystemExit('Refusing diagnostic package with virtual input: ' + name)
output = root / 'dist/PhonePadNX-0.2.0-diagnostic.zip' 
output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as z:
    z.write(root / 'launcher/PhonePadNX-check.nro', 'switch/PhonePadNX-check/PhonePadNX-check.nro')
    z.write(root / 'README.md', 'README.md')
print('Created', output.name)
