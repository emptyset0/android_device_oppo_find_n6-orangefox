#!/usr/bin/env python3
"""Apply the published Find N6 source changes to the pinned Fox 16 tree."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tree', type=Path)
    parser.add_argument('stock_boot', type=Path, help='your own matching OEM boot.img')
    args = parser.parse_args()
    tree = args.tree.resolve()
    repo = Path(__file__).resolve().parents[1]
    if repo != tree / 'device/oppo/plp110':
        raise SystemExit('Clone this device tree to device/oppo/plp110 inside the synced Fox tree.')
    pins = json.loads((repo / 'manifest/project-pins.json').read_text())
    patches = []
    for project, pin in pins.items():
        git = ['git', '-C', str(tree / project)]
        head = subprocess.check_output(git + ['rev-parse', 'HEAD'], text=True).strip()
        if head != pin['revision']:
            raise SystemExit(f'{project}: unexpected HEAD {head}')
        if 'patch' in pin:
            patch = repo / pin['patch']
            subprocess.run(git + ['apply', '--check', str(patch)], check=True)
            patches.append((git, patch))
    header = args.stock_boot.read_bytes()
    if header[:8] != b'ANDROID!' or struct.unpack_from('<I', header, 40)[0] != 4:
        raise SystemExit('Expected an Android boot v4 image.')
    length = struct.unpack_from('<I', header, 8)[0]
    blob = header[4096:4096 + length]
    if hashlib.sha256(blob).hexdigest() != 'e2efc9dcae2d180e22c85b5d760776b232178dfa758784cf94df3bc2df39147f':
        raise SystemExit('Boot kernel differs from the published build input; see the build notes.')
    extra = tree / 'bootable/recovery/otg_sysfs_match.h'
    if extra.exists():
        raise SystemExit(f'Refusing to replace {extra}')
    for git, patch in patches:
        subprocess.run(git + ['apply', str(patch)], check=True)
    extra.write_bytes((repo / 'patches/otg_sysfs_match.h').read_bytes())
    target = repo / 'prebuilt/kernel'
    target.parent.mkdir(exist_ok=True)
    target.write_bytes(blob)


if __name__ == '__main__':
    main()
