# Build and package

The release uses a pinned Android 16 OrangeFox tree. The versions and patch paths are in `manifest/fox16.xml` and `manifest/project-pins.json`. Use a Linux build host with the AOSP build prerequisites (including JDK 17, `python` mapped to Python 3, and `libdeflate.so.0` for repacking).

```sh
mkdir fox16 && cd fox16
repo init -u https://github.com/emptyset0/android_device_oppo_find_n6-orangefox -m manifest/fox16.xml -b main
repo sync -c
git clone https://github.com/emptyset0/android_device_oppo_find_n6-orangefox device/oppo/swangoose
python3 device/oppo/swangoose/tools/apply-source.py "$PWD" /path/to/oem/boot.img
source build/envsetup.sh
source device/oppo/swangoose/vendorsetup.sh
lunch twrp_swangoose-bp2a-eng
m -j4 recovery
```

`apply-source.py` extracts the exact kernel input from the matching APC A.40 OEM `boot.img`. Its SHA-256 is recorded in the script. The kernel runs from the device's existing boot partition, not from the packaged recovery. The raw AOSP `recoveryimage` is **not** the downloadable N6 image.

The release uses a consolidated, N6-specific ramdisk containing the already integrated OEM HAL/runtime assets, X9U-style recovery payload and the source-built core. Use the published, verified `.img` release asset as the seed. Repacking also migrates legacy Recovery product fields to `swangoose`; OEM fingerprints, the OTA partition list, other runtime files, and the original AVB envelope are preserved.

```sh
version=dev
build_date=$(date +%Y%m%d)
product_out=out/target/product/swangoose/recovery/root
# Build the HAL as described in docs/CRYPTO-HAL.md before repacking.
python3 device/oppo/swangoose/tools/repack-recovery.py \
  /path/to/orangefox_r12.0_oppo_find_n6_swangoose_v0.1.1_20261112.img \
  "$product_out/system/bin/recovery" \
  "orangefox_r12.0_oppo_find_n6_swangoose_${version}_${build_date}.img" \
  --crypto-hal /path/to/rebuilt/crypto-hal \
  --minuitwrp "$product_out/system/lib64/libminuitwrp.so"
```

Use the Recovery copy of `libminuitwrp.so` from the same build as the executable.
Without `--minuitwrp`, the repacker preserves the seed's DRM implementation.

For the optional CPU-temperature build, run `python3 device/oppo/swangoose/tools/apply_cpu_temperature.py "$PWD"` after `apply-source.py` and before building. This only changes the display data source.

To remove empty Magisk add-on entries from an older N6 release seed, pass `--clean-menu` to the repacker. This preserves `magiskboot`, native ZIP installation and the manager for a real installed Magisk setup with modules. It does not add KernelSU.

This workflow reproduces the packaging from the release seed; it does not regenerate the entire OEM-dependent ramdisk from bare upstream sources. Private ramdisk service names, properties and paths such as `sys.plp110.keymint` are deliberately kept in sync with the seed. The project does not include an OPPO ROM, keys, or a partition dump. Replacing native libraries, HALs, init or filesystem metadata requires auditing and packaging those assets separately, not just replacing the core executable.
