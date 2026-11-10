#!/usr/bin/env python3
"""Use upstream KernelSU 3.3.1 to patch an N6 init_boot file offline, never a device."""
import argparse
import hashlib
import os
from pathlib import Path
import stat
import struct
import subprocess
import tempfile


TOOL = 'ksud-x86_64-pc-windows-gnu.exe'
TOOL_SHA = 'c259b0111a4fc6ab21f0f1a6d56b66cf87b377e3f87325340c8221fff904b88f'
MODULE = 'lkm-aarch64-android16-6.12_kernelsu.ko'
MODULE_SHA = '7e25d68419d08535d35e3630d9f0ff1a08e97a7026d36e878991eecb13554d9f'


def check_sha(path, digest):
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit(f'{path.name}: does not match the pinned upstream asset.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stock_init_boot', type=Path)
    parser.add_argument('upstream_assets', type=Path)
    parser.add_argument('output_image', type=Path)
    args = parser.parse_args()
    image, directory, output = (p.resolve() for p in (args.stock_init_boot, args.upstream_assets, args.output_image))
    tool, module = directory / TOOL, directory / MODULE
    check_sha(tool, TOOL_SHA)
    check_sha(module, MODULE_SHA)
    if not stat.S_ISREG(image.stat().st_mode):
        raise SystemExit('Provide a regular OEM init_boot image file.')
    raw = image.read_bytes()
    if (raw[:8] != b'ANDROID!' or struct.unpack_from('<I', raw, 8)[0] != 0 or
            struct.unpack_from('<I', raw, 12)[0] == 0 or struct.unpack_from('<I', raw, 40)[0] != 4):
        raise SystemExit('Expected an Android boot v4 image with ramdisk and no kernel (init_boot).')
    if output.exists():
        raise SystemExit(f'Refusing to overwrite {output}')
    with tempfile.TemporaryDirectory(prefix='ksu-n6-', dir=output.parent) as tmp:
        work = Path(tmp)
        stock = work / 'stock.img'
        stock.write_bytes(raw)
        subprocess.run([str(tool), 'boot-patch', '--boot', str(stock), '--kmi', 'android16-6.12',
                        '--module', str(module), '--out', str(work), '--out-name', 'patched.img'],
                       check=True, env=dict(os.environ, TMP=tmp, TEMP=tmp))
        patched = (work / 'patched.img').read_bytes()
        if len(patched) != len(raw) or patched == raw or not patched.startswith(b'ANDROID!'):
            raise SystemExit('Unexpected patched image; output not written.')
        with output.open('xb') as stream:
            stream.write(patched)
    print(f'{hashlib.sha256(patched).hexdigest()}  {output}')
    print('Offline output only. The original locked/green AVB chain does not accept the changed init_boot hash.')


if __name__ == '__main__':
    main()
