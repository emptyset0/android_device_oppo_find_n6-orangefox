# OrangeFox R12 for OPPO Find N6 — v0.1.3

## Changes

- Read the installed system's Android release in the private KeyMint view. This fixes Android 17 metadata decryption when boot still reports Android 16; Android 16 remains supported.
- Load `nxp-nci.ko` through Fox's native vendor-module loader from the current firmware, like the X9U reference. Remove the fixed-version module from the Recovery ramdisk and wait for the eSE device before starting its service.
- Keep the original Recovery AVB envelope and the existing N6 restore, OTA and boot-chain behavior.

## Validation

On PLP110 with CPH2765 Android 17, the image was flashed and read back on both Recovery slots. Fox loaded the active firmware's NFC/eSE module, all eSE/OMAPI/Weaver services remained running, KeyMint received Android 17, metadata decryption succeeded, and user 0 CE decryption was confirmed on the phone. `/data` mounted read/write. A full backup/restore and cross-version OTA were not retested for this release.

The flashable file is the attached `.img`, not GitHub's automatic source archive. Check its hash against the attached `sha256sums.txt`. This is a seed-based repack using OEM runtime files; see `docs/BUILD.md` and `build-provenance.json`.
