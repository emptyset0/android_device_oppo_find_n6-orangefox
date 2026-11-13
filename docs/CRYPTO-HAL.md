# OEM crypto HAL launcher

The launcher retains the private OEM namespace, but releases its inherited
Metadata mount so Recovery can unmount and format Metadata when requested.
Build it with the Android NDK from the device-tree root:

```sh
aarch64-linux-android34-clang -Os -Wall -Wextra -Werror -Wl,--strip-all \
  src/plp110_crypto_hal.c -lbinder_ndk -ldl -o crypto-hal
```

Pass `--crypto-hal crypto-hal` when repacking a verified Find N6 seed.
