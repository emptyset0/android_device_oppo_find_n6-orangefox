# OEM crypto HAL launcher

The launcher retains the private OEM namespace, but releases its inherited
Metadata mount so Recovery can unmount and format Metadata when requested.
Build it with Android NDK r29 (29.0.14206865) from the device-tree root:

```sh
aarch64-linux-android34-clang -Os -Wall -Wextra -Werror -Wl,--strip-all \
  src/plp110_crypto_hal.c -lbinder_ndk -ldl -o crypto-hal
```

Pass `--crypto-hal crypto-hal` to the [release packaging command](BUILD.md).
