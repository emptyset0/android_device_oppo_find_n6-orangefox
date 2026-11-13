#!/usr/bin/env python3
"""Rebuild the Fox core in a known N6 release envelope. No device access."""
import argparse
import ctypes as C
import gzip
import hashlib
from pathlib import Path
import re
import stat
import struct


START = 4096
LIMIT = 35880960
SIZE = 104857600
CORE = 'system/bin/recovery'
SUMS = 'ramdisk-files.sha256sum'
PROPS = 'prop.default'
HAL = 'system/libexec/plp110-fbe/bin/hal'
MINUI = 'system/lib64/libminuitwrp.so'
RECOVERY_PRODUCT_PROP = re.compile(
    rb'ro\.product(?:\.(?:system|vendor|odm|product|system_ext|system_dlkm|vendor_dlkm|odm_dlkm))?'
    rb'\.(device|model|name)'
)
LEGACY_IDENTITY = {
    (b'device', b'plp110'): b'swangoose',
    (b'model', b'PLP110'): b'Find N6',
    (b'name', b'fox_plp110'): b'fox_swangoose',
    (b'name', b'twrp_plp110'): b'twrp_swangoose',
}


def update_recovery_identity(data):
    """Rename only legacy Recovery product fields in a v0.1.0 seed."""
    def replace(m):
        key, old = m.groups()
        prop = RECOVERY_PRODUCT_PROP.fullmatch(key)
        new = LEGACY_IDENTITY.get((prop[1], old)) if prop else None
        if key == b'ro.build.product' and old == b'plp110':
            new = b'swangoose'
        return key + b'=' + new if new is not None else m[0]

    return re.sub(rb'^([^=\r\n]+)=([^\r\n]*)', replace, data, flags=re.M)


def align(value, size):
    return (value + size - 1) & ~(size - 1)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def newc(data):
    offset = 0
    while offset + 110 <= len(data):
        assert data[offset:offset+6] == b'070701'
        fields = [int(data[i:i+8], 16) for i in range(offset+6, offset+110, 8)]
        size, namesize = fields[6], fields[11]
        name = data[offset+110:offset+109+namesize].decode()
        start = align(offset+110+namesize, 4)
        end = align(start+size, 4)
        assert end <= len(data) and namesize and data[offset+109+namesize] == 0
        yield name, fields, data[start:start+size], data[offset:end]
        if name == 'TRAILER!!!':
            assert not any(data[end:])
            return
        offset = end
    raise ValueError('No CPIO trailer')


def entry(name, fields, data):
    name = name.encode() + b'\0'
    fields = fields.copy()
    fields[6], fields[11] = len(data), len(name)
    head = b'070701' + ''.join(f'{n:08x}' for n in fields).encode() + name
    return head.ljust(align(len(head), 4), b'\0') + data.ljust(align(len(data), 4), b'\0')


def compress(data):
    ld = C.CDLL('libdeflate.so.0')
    ld.libdeflate_alloc_compressor.argtypes = [C.c_int]
    ld.libdeflate_alloc_compressor.restype = C.c_void_p
    ld.libdeflate_free_compressor.argtypes = [C.c_void_p]
    ld.libdeflate_gzip_compress_bound.argtypes = [C.c_void_p, C.c_size_t]
    ld.libdeflate_gzip_compress_bound.restype = C.c_size_t
    ld.libdeflate_gzip_compress.argtypes = [C.c_void_p, C.c_char_p, C.c_size_t, C.c_void_p, C.c_size_t]
    ld.libdeflate_gzip_compress.restype = C.c_size_t
    obj = ld.libdeflate_alloc_compressor(12)
    assert obj
    try:
        out = C.create_string_buffer(ld.libdeflate_gzip_compress_bound(obj, len(data)))
        size = ld.libdeflate_gzip_compress(obj, data, len(data), out, len(out))
        assert size
        return out.raw[:size]
    finally:
        ld.libdeflate_free_compressor(obj)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base_recovery', type=Path, help='the released N6 Fox image, not the raw build output')
    parser.add_argument('recovery_elf', type=Path)
    parser.add_argument('output_image', type=Path)
    parser.add_argument('--clean-menu', action='store_true', help='hide unbundled Magisk add-on choices')
    parser.add_argument('--crypto-hal', type=Path, help='replace the N6 OEM HAL launcher with a tested build')
    parser.add_argument('--minuitwrp', type=Path, help='replace the rebuilt minuitwrp DRM backend')
    args = parser.parse_args()
    base = args.base_recovery.read_bytes()
    core = args.recovery_elf.read_bytes()
    assert len(base) == SIZE and base[:8] == b'ANDROID!' and base[-64:-60] == b'AVBf'
    kernel, length = struct.unpack_from('<II', base, 8)
    assert kernel == 0 and 0 < length < LIMIT - START
    assert base[START:START+6] == b'\0\0\0\0\x1f\x8b'
    records = list(newc(gzip.decompress(base[START+4:START+length])))
    by_name = {n: (f,d) for n,f,d,_ in records}
    assert len(by_name) == len(records) and core[:20] == by_name[CORE][1][:20] and core[:4] == b'\x7fELF'
    replacement = {CORE: core}
    if args.crypto_hal:
        hal = args.crypto_hal.read_bytes()
        old_hal = by_name[HAL][1]
        assert hal[:4] == b'\x7fELF' and hal[:20] == old_hal[:20]
        replacement[HAL] = hal
    if args.minuitwrp:
        minui = args.minuitwrp.read_bytes()
        assert minui[:4] == b'\x7fELF' and minui[:20] == by_name[MINUI][1][:20]
        replacement[MINUI] = minui
    if PROPS in by_name:
        old_props = by_name[PROPS][1]
        new_props = update_recovery_identity(old_props)
        if new_props != old_props:
            replacement[PROPS] = new_props
    if args.clean_menu:
        from hide_unavailable_magisk import patch_advanced
        advanced = 'twres/pages/advanced.xml'
        replacement[advanced] = patch_advanced(by_name[advanced][1])
    checksums = b''.join(f'{sha(replacement.get(n, d))}  ./{n}\n'.encode()
                         for n,f,d,_ in records if stat.S_ISREG(f[1]) and n not in (SUMS, PROPS))
    replacement[SUMS] = checksums
    cpio = b''.join(entry(n,f,replacement[n]) if n in replacement else raw for n,f,d,raw in records)
    cpio = cpio.ljust(align(len(cpio),512), b'\0')
    assert {n:d for n,_,d,_ in newc(cpio)} == {n:replacement.get(n,d) for n,(_,d) in by_name.items()}
    packed = b'\0'*4 + compress(cpio)
    end = align(START + len(packed), START)
    if end > LIMIT:
        raise SystemExit(f'Ramdisk exceeds N6 gzip payload limit by {end-LIMIT} bytes.')
    assert gzip.decompress(packed[4:]) == cpio
    image = bytearray(base)
    struct.pack_into('<I', image, 12, len(packed))
    image[START:LIMIT] = packed.ljust(LIMIT-START, b'\0')
    assert image[:12] == base[:12] and image[16:START] == base[16:START] and image[LIMIT:] == base[LIMIT:]
    with args.output_image.open('xb') as f:
        f.write(image)
    print(f'{sha(image)}  {args.output_image} ({LIMIT-end} bytes spare)')


if __name__ == '__main__':
    main()
