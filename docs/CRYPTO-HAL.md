# OEM crypto HAL launcher

The launcher retains the private OEM namespace, but releases its inherited
Metadata mount so Recovery can unmount and format Metadata when requested.
Build it with Android NDK r29 (29.0.14206865) from the device-tree root:

```sh
aarch64-linux-android34-clang -Os -Wall -Wextra -Werror -Wl,--strip-all \
  src/plp110_crypto_hal.c -lbinder_ndk -ldl -o crypto-hal
```

Pass `--crypto-hal crypto-hal` to the [release packaging command](BUILD.md).

The release requires a rebuilt Recovery core with `TW_LOAD_VENDOR_MODULES`.
The Fox loader takes `nxp-nci.ko` from the active firmware, and the eSE
launcher waits for its character device. Packaging removes the obsolete
seed copy and replaces the seed startup script. A missing driver must not
prevent KeyMint, metadata decryption, or Recovery itself from starting.

Fox's early vendor-module loader runs synchronously in a short-lived
worker with a private mount namespace. Its temporary mounts never enter
the main Recovery mount view, so QSEE does not need a mountpoint denylist
for module loading. The QSEE jail itself is unchanged.

The separate private HAL cleanup of `/metadata` is still needed: native
FBE and Virtual A/B use it outside the module loader. The firmware-release
reader also still unmounts the temporary mount it creates itself.

The worker replacement was cold-boot tested on PLP110: module loading,
QSEE/KeyMint, native Data decryption and read-write Data mount succeeded.
The standard F2FS Format Data path then formatted `/data` and `/metadata`
without errors (`operation_end - status=0`). Keep the global `rm -rf` option
off for Format Data; it does not format a filesystem or remove encryption.
