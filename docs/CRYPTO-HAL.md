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

Before entering the QSEE jail, the launcher drops inherited
`/vendor_dlkm`, `/vendor` and `/system_root` mounts in a private view.
This keeps Fox's transient module-loading mounts from pinning the logical
devices during the native Format Data snapshot preflight.

On the tested Android 17 firmware, the QSEE, KeyMint and Data decryption
services remained available after this change. The standard F2FS Format Data
path completed for both `/data` and `/metadata`, followed by successful
Android and Recovery boots. The global `rm -rf` wipe option was off for that
format test; it does not perform a filesystem format or remove encryption.
