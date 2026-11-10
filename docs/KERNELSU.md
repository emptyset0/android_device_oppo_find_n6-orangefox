# KernelSU on Find N6

The tested APC A.40 kernel is `6.12.38-android16-5-…-4k` (arm64, 4 KiB). The official LKM family is `android16-6.12`; this is not an end-to-end module-load result. Current upstream KernelSU uses LKM mode, retaining the OEM kernel. See the [official installation guide](https://kernelsu.org/guide/installation.html).

## Offline file preparation, not an installation

On Windows, download `ksud-x86_64-pc-windows-gnu.exe` and `lkm-aarch64-android16-6.12_kernelsu.ko` from [KernelSU v3.3.1](https://github.com/tiann/KernelSU/releases/tag/v3.3.1) into the same directory. Run:

```powershell
python tools/patch-kernelsu-init-boot.py .\oem\init_boot.img .\ksu-assets .\init_boot-patched.img
```

This was exercised on the APC A.40 `init_boot` image; it produces a separate image and does not write to the phone. The patched payload fails the original `init_boot` AVB hash. Do not treat a `locked/green` Canoe setup that accepts this Recovery as proof it accepts a patched `init_boot`. No KernelSU write, boot, or root result has been tested on this configuration. On a genuinely unlocked configuration, follow upstream instructions for selecting the correct slot and recovering the original image.

## Why it is not built in

The signed-envelope gzip budget leaves **1,671,168 bytes**. Testing on the actual N6 archive:

| Proposed additions | Result after 4 KiB alignment |
| --- | ---: |
| X9U's packed `ksud` and three reference ZIPs | 2,355,200 bytes over |
| Upstream Android `ksud` plus only the N6 LKM ZIP | 2,396,160 bytes over |

The recovery therefore does not ship `ksud`, a non-functional menu item, or an automatic root operation. The offline companion uses matching, upstream-pinned tools without consuming ramdisk space or removing recovery features.
