#!/usr/bin/env python3
"""Create and verify a Find N6 release asset from a pinned, verified seed."""
import argparse
from datetime import date
import gzip
import importlib
import json
from pathlib import Path
import platform
import re
import stat
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
pack = importlib.import_module("repack-recovery")
ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return pack.sha(path.read_bytes())


def contents(image):
    end = pack.START + struct.unpack_from("<I", image, 12)[0]
    entries = list(pack.newc(gzip.decompress(image[pack.START + 4:end])))
    return {n: (fields, data) for n, fields, data, _ in entries}


def verify(seed, image, replacements):
    if not (len(seed) == len(image) == pack.SIZE and seed[:12] == image[:12]
            and seed[16:pack.START] == image[16:pack.START] and seed[pack.LIMIT:] == image[pack.LIMIT:]):
        raise ValueError("Recovery image envelope changed")
    end = pack.align(pack.START + struct.unpack_from("<I", image, 12)[0], pack.START)
    if end > pack.LIMIT:
        raise ValueError("Packed ramdisk exceeds the firmware limit")
    old, new = contents(seed), contents(image)
    if old.keys() != new.keys():
        raise ValueError("Ramdisk file list changed")
    for name, (fields, data) in old.items():
        actual_fields, actual_data = new[name]
        if name not in replacements and name != pack.SUMS and (actual_fields, actual_data) != (fields, data):
            raise ValueError(f"Unexpected ramdisk change: {name}")
    for name, expected in replacements.items():
        if new[name][1] != expected:
            raise ValueError(f"Packaged payload differs: {name}")
    expected_ledger = b"".join(f"{pack.sha(data)}  ./{name}\n".encode()
        for name, (fields, data) in new.items()
        if stat.S_ISREG(fields[1]) and name not in (pack.SUMS, pack.PROPS))
    if new[pack.SUMS][1] != expected_ledger:
        raise ValueError("Ramdisk checksum ledger does not match its contents")
    return pack.LIMIT - end


def version(cmd):
    return subprocess.check_output(cmd, text=True).splitlines()[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ("seed", "core", "minuitwrp", "crypto-hal", "oem-boot", "ndk-properties",
                 "android-clang", "output-dir"):
        ap.add_argument(f"--{name}", required=True, type=Path)
    ap.add_argument("--version", required=True, help="version such as v0.1.2")
    ap.add_argument("--date", required=True, help="build date, YYYYMMDD")
    a = ap.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.-]+)?", a.version):
        ap.error("invalid release version")
    if not re.fullmatch(r"[0-9]{8}", a.date):
        ap.error("date must be YYYYMMDD")
    date.fromisoformat(a.date)
    if a.output_dir.exists():
        ap.error("output directory must not exist")
    seed_info = json.loads((ROOT / "manifest/release-seed.json").read_text())
    seed = a.seed.read_bytes()
    if pack.sha(seed) != seed_info["sha256"]:
        ap.error("seed SHA-256 does not match manifest/release-seed.json")
    boot = a.oem_boot.read_bytes()
    if len(boot) < 4096 or boot[:8] != b"ANDROID!" or struct.unpack_from("<I", boot, 40)[0] != 4:
        ap.error("OEM input must be an Android boot v4 image")
    length = struct.unpack_from("<I", boot, 8)[0]
    kernel_sha = pack.sha(boot[4096:4096 + length])
    if kernel_sha != "e2efc9dcae2d180e22c85b5d760776b232178dfa758784cf94df3bc2df39147f":
        ap.error("OEM boot kernel does not match the pinned build input")
    ndk = re.search(r"(?m)^Pkg\.Revision\s*=\s*(\S+)", a.ndk_properties.read_text())
    if not ndk:
        ap.error("invalid NDK source.properties")
    if ndk[1] != "29.0.14206865":
        ap.error("release HAL build requires Android NDK r29 (29.0.14206865)")
    inputs = {pack.CORE: a.core, pack.MINUI: a.minuitwrp, pack.HAL: a.crypto_hal}
    replacements = {name: path.read_bytes() for name, path in inputs.items()}
    source = version(["git", "-C", ROOT, "rev-parse", "HEAD"])
    if subprocess.check_output(["git", "-C", ROOT, "status", "--porcelain"], text=True).strip():
        ap.error("package from a clean device-tree commit")
    provenance = {
        "version": a.version, "build_date": a.date, "device_tree_commit": source,
        "seed": seed_info,
        "oem_boot_sha256": pack.sha(boot), "oem_kernel_sha256": kernel_sha,
        "inputs": {n: pack.sha(data) for n, data in replacements.items()},
        "toolchain": {"ndk_revision": ndk[1], "android_clang": version([a.android_clang, "--version"]),
                      "python": platform.python_version(),
                      "libdeflate_package": version(["dpkg-query", "-W", "-f=${Version}\n", "libdeflate0"])},
        "manifest_sha256": digest(ROOT / "manifest/fox16.xml"),
        "project_pins_sha256": digest(ROOT / "manifest/project-pins.json"),
    }
    name = f"orangefox_r12.0_oppo_find_n6_swangoose_{a.version}_{a.date}.img"
    with tempfile.TemporaryDirectory() as directory:
        image = Path(directory) / name
        subprocess.run([sys.executable, "-B", ROOT / "tools/repack-recovery.py", a.seed,
                        a.core, image, "--minuitwrp", a.minuitwrp, "--crypto-hal", a.crypto_hal], check=True)
        result = image.read_bytes()
        spare = verify(seed, result, replacements)
        provenance.update(image=name, image_sha256=pack.sha(result), ramdisk_bytes_spare=spare)
        a.output_dir.mkdir()
        (a.output_dir / name).write_bytes(result)
        (a.output_dir / "sha256sums.txt").write_text(f"{provenance['image_sha256']}  {name}\n")
        (a.output_dir / "build-provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Verified release: {(a.output_dir / name).resolve()} ({spare} bytes spare)")


if __name__ == "__main__":
    main()
